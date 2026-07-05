import paho.mqtt.client as mqtt
import json
import threading
from config import MQTT_HOST, MQTT_PORT, MQTT_TOPICS


class MQTTClient:
    def __init__(self):
        self.results = {}
        self.lock = threading.Lock()
        self.client = mqtt.Client()
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect
        self._connected = False
        try:
            self.client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
            self.client.loop_start()
        except Exception as e:
            print(f"[MQTT] 连接失败: {e}，将以离线模式运行")

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self._connected = True
            print(f"[MQTT] 已连接到 {MQTT_HOST}:{MQTT_PORT}")
            client.subscribe(MQTT_TOPICS["results"])
        else:
            print(f"[MQTT] 连接失败，返回码: {rc}")

    def _on_disconnect(self, client, userdata, rc):
        self._connected = False
        print("[MQTT] 已断开连接")

    def _on_message(self, client, userdata, msg):
        try:
            with self.lock:
                self.results = json.loads(msg.payload)
        except json.JSONDecodeError as e:
            print(f"[MQTT] 消息解析失败: {e}")

    def get_results(self):
        with self.lock:
            return self.results.copy()

    def trigger_feed(self):
        if not self._connected:
            print("[MQTT] 未连接，无法发送喂食指令")
            return
        payload = json.dumps({"act": "feed", "source": "manual_test"})
        self.client.publish(MQTT_TOPICS["cmd"], payload)

    def stop(self):
        self.client.loop_stop()
        self.client.disconnect()
