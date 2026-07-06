# 开发与测试工具

项目开发、部署和测试工具集。

## 目录结构

```
tools/
├── deploy.sh              # 一键部署脚本
├── test_rtsp.py           # RTSP 推流测试工具
├── test_mqtt.py           # MQTT 通信测试工具
└── model_conversion/      # 模型转换工具
    ├── convert_yolov5n.py
    ├── convert_reid.py
    ├── convert_mobilenet.py
    ├── requirements.txt
    └── README.md
```

## 一键部署

将编译好的程序部署到 RV1106 设备。

### 使用方法

```bash
# 默认设备IP: 192.168.1.100
./deploy.sh

# 指定设备IP
./deploy.sh 192.168.1.200
```

### 部署内容

- `rv1106-feeder/build/pet_feeder` → `/userdata/bin/`
- `rv1106-feeder/config/*.json` → `/userdata/config/`
- `rv1106-feeder/models/*.rknn` → `/userdata/models/`

## RTSP 推流测试

测试 RV1106 设备的 RTSP 推流功能。

### 使用方法

```bash
# 默认地址: rtsp://192.168.1.100:554/live/0
python3 test_rtsp.py

# 指定地址和测试时长
python3 test_rtsp.py --url rtsp://192.168.1.200:554/live/0 --duration 60

# 保存测试帧
python3 test_rtsp.py --save-frame test.jpg
```

### 快捷键

- `Q` — 退出测试
- `S` — 保存当前帧

### 测试指标

- 实际帧率
- 丢帧率
- 推流稳定性

## MQTT 通信测试

测试 RV1106 设备的 MQTT 通信功能。

### 使用方法

```bash
# 监听消息（默认30秒）
python3 test_mqtt.py --broker 192.168.1.100

# 发布测试指令并监听
python3 test_mqtt.py --broker 192.168.1.100 --publish --duration 60
```

### 订阅主题

- `pet_feeder/feeding` — 喂食事件
- `pet_feeder/eating` — 进食事件
- `pet_feeder/food_level` — 余量上报
- `pet_feeder/alarm` — 告警通知
- `pet_feeder/status` — 状态上报

### 发布主题

- `pet_feeder/cmd` — 控制指令

## 模型转换

参见 [model_conversion/README.md](model_conversion/README.md)

## 完整测试流程

### 1. 编译程序

```bash
cd rv1106-feeder
./build.sh
cd ..
```

### 2. 转换模型

```bash
cd tools/model_conversion

# YOLOv5n
python3 convert_yolov5n.py \
    --weight yolov5n.pt \
    --output ../../rv1106-feeder/models/yolov5n.rknn

# Re-ID
python3 convert_reid.py \
    --model osnet_x0_25.pth \
    --output ../../rv1106-feeder/models/reid.rknn

# MobileNetV2
python3 convert_mobilenet.py \
    --model food_classifier.pth \
    --output ../../rv1106-feeder/models/mobilenet.rknn

cd ../..
```

### 3. 部署到设备

```bash
./tools/deploy.sh 192.168.1.100
```

### 4. SSH 登录设备并运行

```bash
ssh root@192.168.1.100
cd /userdata/bin
./pet_feeder
```

### 5. 本地测试

**终端1: 测试 RTSP 推流**

```bash
python3 tools/test_rtsp.py --url rtsp://192.168.1.100:554/live/0
```

**终端2: 测试 MQTT 通信**

```bash
python3 tools/test_mqtt.py --broker 192.168.1.100 --publish
```

**终端3: 运行 Python 上位机**

```bash
cd pet-feeder-monitor
python3 main.py
```

## 故障排查

### 部署失败

```bash
# 检查设备网络
ping 192.168.1.100

# 检查SSH连接
ssh root@192.168.1.100

# 检查磁盘空间
ssh root@192.168.1.100 'df -h'
```

### RTSP 推流失败

```bash
# 检查设备进程
ssh root@192.168.1.100 'ps | grep pet_feeder'

# 检查端口占用
ssh root@192.168.1.100 'netstat -tulnp | grep 554'

# 查看设备日志
ssh root@192.168.1.100 'dmesg | tail -50'
```

### MQTT 连接失败

```bash
# 检查 MQTT Broker 状态
systemctl status mosquitto

# 测试 MQTT Broker
mosquitto_sub -h 192.168.1.100 -t 'pet_feeder/#' -v
```

## 依赖库

### Python 工具

```bash
pip install opencv-python paho-mqtt numpy
```

### 模型转换

```bash
pip install -r model_conversion/requirements.txt
```

## 相关文档

- [开发任务计划](../docs/开发任务计划.md)
- [rv1106-feeder README](../rv1106-feeder/README.md)
- [宠物喂食器技术方案](../docs/宠物喂食器技术方案.md)
