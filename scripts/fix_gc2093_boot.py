#!/usr/bin/env python3
"""Patch a Luckfox external-data FIT boot image for the Pico Zero GC2093.

The script keeps every external payload at its original position and size.  It
only replaces the embedded DTB (and its resource copy), then refreshes the FIT
SHA-256 values.  This avoids changing Rockchip's non-standard kernel load and
entry values.
"""

from __future__ import annotations

import argparse
import hashlib
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path


FDT_MAGIC = b"\xd0\x0d\xfe\xed"


def run(*args: str, check: bool = True) -> str:
    result = subprocess.run(args, check=check, text=True, capture_output=True)
    return result.stdout.strip()


def fit_number(image: Path, node: str, prop: str) -> int:
    return int(run("fdtget", "-t", "x", str(image), node, prop), 16)


def fit_hash(image: Path, node: str) -> bytes:
    values = run("fdtget", "-t", "bx", str(image), node, "value").split()
    return bytes(int(value, 16) for value in values)


def remove_node(dtb: Path, node: str) -> None:
    subprocess.run(
        ("fdtput", "-r", str(dtb), node),
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def replace_once(data: bytearray, old: bytes, new: bytes, limit: int) -> None:
    positions = []
    start = 0
    while True:
        pos = data.find(old, start, limit)
        if pos < 0:
            break
        positions.append(pos)
        start = pos + 1
    if len(positions) != 1:
        raise RuntimeError(f"expected one FIT hash occurrence, found {len(positions)}")
    data[positions[0] : positions[0] + len(old)] = new


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    for tool in ("fdtget", "fdtput"):
        if shutil.which(tool) is None:
            raise SystemExit(f"missing required tool: {tool}")

    source = args.input.resolve()
    output = args.output.resolve()
    image = bytearray(source.read_bytes())
    fit_size = struct.unpack(">I", image[4:8])[0]
    fdt_pos = fit_number(source, "/images/fdt", "data-position")
    fdt_size = fit_number(source, "/images/fdt", "data-size")
    resource_pos = fit_number(source, "/images/resource", "data-position")
    resource_size = fit_number(source, "/images/resource", "data-size")

    old_fdt = bytes(image[fdt_pos : fdt_pos + fdt_size])
    old_resource = bytes(image[resource_pos : resource_pos + resource_size])
    old_fdt_hash = fit_hash(source, "/images/fdt/hash")
    old_resource_hash = fit_hash(source, "/images/resource/hash")
    if not old_fdt.startswith(FDT_MAGIC):
        raise RuntimeError("FIT FDT payload has no FDT magic")
    if old_fdt_hash != hashlib.sha256(old_fdt).digest():
        raise RuntimeError("input FIT has an invalid FDT hash")

    with tempfile.TemporaryDirectory() as tmp:
        dtb = Path(tmp) / "gc2093.dtb"
        dtb.write_bytes(old_fdt)

        for node in (
            "/i2c@ff470000/ov5647@36",
            "/i2c@ff470000/imx415@37",
        ):
            remove_node(dtb, node)

        run("fdtput", "-t", "x", str(dtb), "/out-osc-imx415", "clock-frequency", "0x02367b88")
        # RV1106's local DPHY endpoint describes the four-lane receiver block;
        # the remote GC2093 endpoint remains the actual two-lane sensor link.
        run("fdtput", "-t", "x", str(dtb), "/csi2-dphy0/ports/port@0/endpoint@1", "data-lanes", "1", "2", "3", "4")
        run("fdtput", "-t", "s", str(dtb), "/i2c@ff470000/gc2093@37", "status", "okay")

        patched = dtb.read_bytes()
        if len(patched) > fdt_size:
            raise RuntimeError(f"patched DTB grew beyond FIT slot: {len(patched)} > {fdt_size}")
        new_fdt = patched.ljust(fdt_size, b"\0")

        if old_resource.count(old_fdt) != 1:
            raise RuntimeError("resource payload does not contain exactly one DTB copy")
        new_resource = old_resource.replace(old_fdt, new_fdt, 1)

    image[fdt_pos : fdt_pos + fdt_size] = new_fdt
    image[resource_pos : resource_pos + resource_size] = new_resource
    replace_once(image, old_fdt_hash, hashlib.sha256(new_fdt).digest(), fit_size)
    replace_once(image, old_resource_hash, hashlib.sha256(new_resource).digest(), fit_size)

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(image)

    with tempfile.NamedTemporaryFile(suffix=".dtb") as check:
        check.write(new_fdt)
        check.flush()
        assert run("fdtget", "-t", "x", check.name, "/out-osc-imx415", "clock-frequency") == "2367b88"
        assert run("fdtget", "-t", "x", check.name, "/csi2-dphy0/ports/port@0/endpoint@1", "data-lanes") == "1 2 3 4"
        assert run("fdtget", "-t", "s", check.name, "/i2c@ff470000/gc2093@37", "status") == "okay"

    print(f"wrote {output}")
    print(f"sha256 {hashlib.sha256(image).hexdigest()}")


if __name__ == "__main__":
    main()
