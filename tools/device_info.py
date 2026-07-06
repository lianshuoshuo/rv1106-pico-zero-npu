#!/usr/bin/env python3
"""
设备信息查看工具
用于查看 RV1106 设备的硬件和系统信息
"""

import argparse
import subprocess
import sys


def run_ssh_command(host, user, command):
    """通过 SSH 执行命令"""
    try:
        result = subprocess.run(
            ['ssh', f'{user}@{host}', command],
            capture_output=True,
            text=True,
            timeout=10
        )
        return result.stdout.strip()
    except Exception as e:
        return f"Error: {e}"


def get_device_info(host, user):
    """获取设备信息"""
    print("=" * 70)
    print(f"  RV1106 设备信息查看 - {host}")
    print("=" * 70)

    info = {}

    # 系统信息
    print("\n[系统信息]")
    info['kernel'] = run_ssh_command(host, user, 'uname -r')
    print(f"  内核版本: {info['kernel']}")

    info['uptime'] = run_ssh_command(host, user, 'uptime')
    print(f"  运行时间: {info['uptime']}")

    # CPU 信息
    print("\n[CPU 信息]")
    cpu_info = run_ssh_command(host, user, "cat /proc/cpuinfo | grep 'model name' | head -1")
    if cpu_info:
        print(f"  {cpu_info}")
    else:
        print("  型号: RV1106 (ARM Cortex-A7)")

    cpu_freq = run_ssh_command(host, user, "cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq")
    if cpu_freq and cpu_freq.isdigit():
        print(f"  当前频率: {int(cpu_freq)/1000:.0f} MHz")

    # 内存信息
    print("\n[内存信息]")
    mem_info = run_ssh_command(host, user, "free -h | grep Mem")
    if mem_info:
        parts = mem_info.split()
        print(f"  总量: {parts[1]}")
        print(f"  已用: {parts[2]}")
        print(f"  可用: {parts[6]}")

    # 存储信息
    print("\n[存储信息]")
    disk_info = run_ssh_command(host, user, "df -h /userdata")
    if disk_info:
        lines = disk_info.split('\n')
        if len(lines) > 1:
            parts = lines[1].split()
            print(f"  /userdata 总量: {parts[1]}")
            print(f"  /userdata 已用: {parts[2]}")
            print(f"  /userdata 可用: {parts[3]}")
            print(f"  /userdata 使用率: {parts[4]}")

    # 网络信息
    print("\n[网络信息]")
    ip_info = run_ssh_command(host, user, "ip addr show eth0 | grep 'inet ' | awk '{print $2}'")
    if ip_info:
        print(f"  IP 地址: {ip_info}")

    # 进程信息
    print("\n[进程信息]")
    pet_feeder = run_ssh_command(host, user, "ps | grep pet_feeder | grep -v grep")
    if pet_feeder:
        print(f"  ✓ pet_feeder 正在运行")
        print(f"    {pet_feeder}")
    else:
        print(f"  ✗ pet_feeder 未运行")

    # 端口信息
    print("\n[端口监听]")
    ports = run_ssh_command(host, user, "netstat -tulnp | grep -E ':(554|1883)' | awk '{print $4,$7}'")
    if ports:
        for line in ports.split('\n'):
            if line.strip():
                print(f"  {line}")
    else:
        print("  未检测到 RTSP(554) 或 MQTT(1883) 端口监听")

    # 温度信息（如果可用）
    print("\n[温度信息]")
    temp = run_ssh_command(host, user, "cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null")
    if temp and temp.isdigit():
        print(f"  CPU 温度: {int(temp)/1000:.1f}°C")
    else:
        print("  温度传感器不可用")

    # RKNN 驱动信息
    print("\n[RKNN 驱动]")
    rknn_version = run_ssh_command(host, user, "dmesg | grep -i rknn | head -1")
    if rknn_version:
        print(f"  {rknn_version}")
    else:
        print("  未检测到 RKNN 驱动信息")

    print("\n" + "=" * 70)


def main():
    parser = argparse.ArgumentParser(description='RV1106 设备信息查看工具')
    parser.add_argument('--host', type=str, default='192.168.1.100',
                       help='设备 IP 地址')
    parser.add_argument('--user', type=str, default='root',
                       help='SSH 用户名')

    args = parser.parse_args()

    try:
        get_device_info(args.host, args.user)
    except KeyboardInterrupt:
        print("\n\n用户中断")
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
