#!/usr/bin/env python3
"""
MQTT 测试工具
用于测试 RV1106 设备的 MQTT 通信功能
"""

import argparse
import json
import time
import sys
import paho.mqtt.client as mqtt


class MQTTTester:
    def __init__(self, broker, port, topics):
        self.broker = broker
        self.port = port
        self.topics = topics
        self.client = mqtt.Client()
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect
        self.connected = False
        self.message_count = 0

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected = True
            print(f"✓ 已连接到 MQTT Broker: {self.broker}:{self.port}")
            print(f"订阅主题:")
            for topic in self.topics:
                client.subscribe(topic)
                print(f"  - {topic}")
        else:
            print(f"✗ 连接失败，返回码: {rc}")

    def _on_disconnect(self, client, userdata, rc):
        self.connected = False
        print("✗ 已断开连接")

    def _on_message(self, client, userdata, msg):
        self.message_count += 1
        print(f"\n[{time.strftime('%H:%M:%S')}] 收到消息:")
        print(f"  主题: {msg.topic}")
        print(f"  负载: {msg.payload.decode()}")

        try:
            data = json.loads(msg.payload.decode())
            print(f"  解析: {json.dumps(data, indent=4, ensure_ascii=False)}")
        except json.JSONDecodeError:
            print(f"  (非JSON格式)")

    def connect(self):
        print(f"正在连接 MQTT Broker: {self.broker}:{self.port}...")
        try:
            self.client.connect(self.broker, self.port, keepalive=60)
            self.client.loop_start()
            time.sleep(1)
            return self.connected
        except Exception as e:
            print(f"✗ 连接失败: {e}")
            return False

    def publish(self, topic, payload):
        if not self.connected:
            print("✗ 未连接到 MQTT Broker")
            return False

        try:
            self.client.publish(topic, json.dumps(payload))
            print(f"✓ 发布消息到 {topic}: {payload}")
            return True
        except Exception as e:
            print(f"✗ 发布失败: {e}")
            return False

    def run(self, duration=30):
        print(f"\n监听 {duration} 秒...")
        print("按 Ctrl+C 提前退出")
        print("-" * 60)

        start_time = time.time()

        try:
            while time.time() - start_time < duration:
                time.sleep(0.1)

        except KeyboardInterrupt:
            print("\n用户中断")

        print("\n" + "-" * 60)
        print(f"测试结果:")
        print(f"  收到消息数: {self.message_count}")
        print(f"  运行时间: {time.time() - start_time:.1f} 秒")

    def stop(self):
        self.client.loop_stop()
        self.client.disconnect()


def main():
    parser = argparse.ArgumentParser(description='MQTT 测试工具')
    parser.add_argument('--broker', type=str, default='192.168.1.100',
                       help='MQTT Broker 地址')
    parser.add_argument('--port', type=int, default=1883,
                       help='MQTT Broker 端口')
    parser.add_argument('--duration', type=int, default=30,
                       help='监听时长（秒）')
    parser.add_argument('--publish', action='store_true',
                       help='发布测试指令')

    args = parser.parse_args()

    print("=" * 60)
    print("  MQTT 通信测试工具")
    print("=" * 60)

    # 订阅主题列表
    topics = [
        'pet_feeder/feeding',
        'pet_feeder/eating',
        'pet_feeder/food_level',
        'pet_feeder/alarm',
        'pet_feeder/status'
    ]

    tester = MQTTTester(args.broker, args.port, topics)

    if not tester.connect():
        return 1

    # 如果需要发布测试指令
    if args.publish:
        print("\n发布测试指令...")
        tester.publish('pet_feeder/cmd', {
            'act': 'feed',
            'pet': 'cat_001',
            'source': 'test'
        })
        time.sleep(1)

    # 监听消息
    tester.run(args.duration)

    tester.stop()

    return 0


if __name__ == '__main__':
    sys.exit(main())
