#!/usr/bin/env python3
"""
RTSP 推流测试工具
用于测试 RV1106 设备的 RTSP 推流是否正常
"""

import argparse
import cv2
import time
import sys


def test_rtsp_stream(rtsp_url, duration=30, save_frame=None):
    """测试 RTSP 推流"""
    print(f"正在连接: {rtsp_url}")
    print("按 [Q] 退出，按 [S] 保存当前帧")
    print("-" * 60)

    cap = cv2.VideoCapture(rtsp_url)

    if not cap.isOpened():
        print(f"✗ 无法打开 RTSP 流: {rtsp_url}")
        print("\n可能原因:")
        print("  1. 设备未运行 pet_feeder 程序")
        print("  2. 网络连接问题")
        print("  3. RTSP 端口被占用")
        return False

    print("✓ RTSP 流连接成功")

    # 获取流信息
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = int(cap.get(cv2.CAP_PROP_FPS))

    print(f"  分辨率: {width}x{height}")
    print(f"  帧率: {fps} fps")
    print("-" * 60)

    frame_count = 0
    start_time = time.time()
    last_fps_time = start_time

    cv2.namedWindow('RTSP Stream Test', cv2.WINDOW_NORMAL)

    try:
        while True:
            ret, frame = cap.read()

            if not ret:
                print("✗ 读帧失败，尝试重连...")
                cap.release()
                time.sleep(1)
                cap = cv2.VideoCapture(rtsp_url)
                continue

            frame_count += 1
            current_time = time.time()

            # 计算实际帧率
            if current_time - last_fps_time >= 1.0:
                actual_fps = frame_count / (current_time - start_time)
                cv2.putText(frame, f"FPS: {actual_fps:.1f}", (10, 30),
                           cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                last_fps_time = current_time

            # 叠加信息
            cv2.putText(frame, f"Frame: {frame_count}", (10, 70),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

            cv2.imshow('RTSP Stream Test', frame)

            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('s'):
                filename = f"frame_{frame_count}.jpg"
                cv2.imwrite(filename, frame)
                print(f"保存帧: {filename}")

            # 超时退出
            if duration > 0 and (current_time - start_time) > duration:
                break

    except KeyboardInterrupt:
        print("\n用户中断")

    finally:
        elapsed = time.time() - start_time
        actual_fps = frame_count / elapsed if elapsed > 0 else 0

        print("\n" + "-" * 60)
        print("测试结果:")
        print(f"  总帧数: {frame_count}")
        print(f"  运行时间: {elapsed:.1f} 秒")
        print(f"  平均帧率: {actual_fps:.2f} fps")
        print(f"  丢帧率: {((fps - actual_fps) / fps * 100):.1f}%" if fps > 0 else "  丢帧率: N/A")

        cap.release()
        cv2.destroyAllWindows()

    return True


def main():
    parser = argparse.ArgumentParser(description='RTSP 推流测试工具')
    parser.add_argument('--url', type=str, default='rtsp://192.168.1.100:554/live/0',
                       help='RTSP 流地址')
    parser.add_argument('--duration', type=int, default=0,
                       help='测试时长（秒），0表示无限制')
    parser.add_argument('--save-frame', type=str,
                       help='保存帧的文件名')

    args = parser.parse_args()

    print("=" * 60)
    print("  RTSP 推流测试工具")
    print("=" * 60)

    success = test_rtsp_stream(args.url, args.duration, args.save_frame)

    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())
