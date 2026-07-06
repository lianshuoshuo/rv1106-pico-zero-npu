#!/usr/bin/env python3
"""
性能监控工具
实时监控 RV1106 设备的 CPU、内存、NPU 使用情况
"""

import argparse
import subprocess
import time
import sys
from datetime import datetime


def run_ssh_command(host, user, command):
    """通过 SSH 执行命令"""
    try:
        result = subprocess.run(
            ['ssh', f'{user}@{host}', command],
            capture_output=True,
            text=True,
            timeout=5
        )
        return result.stdout.strip()
    except Exception as e:
        return None


def get_cpu_usage(host, user):
    """获取 CPU 使用率"""
    output = run_ssh_command(host, user, "top -bn1 | grep 'CPU:' | awk '{print $2}'")
    if output:
        try:
            return float(output.replace('%', ''))
        except:
            pass
    return 0.0


def get_memory_usage(host, user):
    """获取内存使用情况"""
    output = run_ssh_command(host, user, "free | grep Mem | awk '{print $3,$2}'")
    if output:
        parts = output.split()
        if len(parts) == 2:
            used = int(parts[0])
            total = int(parts[1])
            return used, total, (used / total * 100) if total > 0 else 0
    return 0, 0, 0


def get_process_info(host, user):
    """获取 pet_feeder 进程信息"""
    output = run_ssh_command(host, user, "ps aux | grep pet_feeder | grep -v grep | awk '{print $3,$4,$11}'")
    if output:
        parts = output.split()
        if len(parts) >= 2:
            return float(parts[0]), float(parts[1])
    return 0.0, 0.0


def monitor(host, user, interval=2):
    """实时监控"""
    print("=" * 70)
    print(f"  RV1106 性能监控 - {host}")
    print("=" * 70)
    print("按 Ctrl+C 退出\n")

    try:
        while True:
            timestamp = datetime.now().strftime("%H:%M:%S")

            # CPU 使用率
            cpu_usage = get_cpu_usage(host, user)

            # 内存使用情况
            mem_used, mem_total, mem_percent = get_memory_usage(host, user)

            # pet_feeder 进程信息
            proc_cpu, proc_mem = get_process_info(host, user)

            # 清屏（可选）
            # print("\033[H\033[J", end="")

            print(f"[{timestamp}] ", end="")
            print(f"CPU: {cpu_usage:5.1f}% | ", end="")
            print(f"MEM: {mem_percent:5.1f}% ({mem_used//1024}MB/{mem_total//1024}MB) | ", end="")

            if proc_cpu > 0 or proc_mem > 0:
                print(f"pet_feeder CPU: {proc_cpu:5.1f}% MEM: {proc_mem:5.1f}%", end="")
            else:
                print("pet_feeder: NOT RUNNING", end="")

            print()

            time.sleep(interval)

    except KeyboardInterrupt:
        print("\n\n监控已停止")


def main():
    parser = argparse.ArgumentParser(description='RV1106 性能监控工具')
    parser.add_argument('--host', type=str, default='192.168.1.100',
                       help='设备 IP 地址')
    parser.add_argument('--user', type=str, default='root',
                       help='SSH 用户名')
    parser.add_argument('--interval', type=int, default=2,
                       help='刷新间隔（秒）')

    args = parser.parse_args()

    monitor(args.host, args.user, args.interval)

    return 0


if __name__ == '__main__':
    sys.exit(main())
