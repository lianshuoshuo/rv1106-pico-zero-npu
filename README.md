# RV1106 Pico Zero NPU — 智能宠物喂食器

基于 Luckfox Pico Zero（RV1106）开发的智能宠物喂食器项目，集成 NPU 推理、视频监控与远程控制。

## 项目结构

```
.
├── docs/                   # 开发文档（14篇）
│   ├── 宠物喂食器技术方案.md    # 完整技术方案
│   ├── 开发任务计划.md          # 7阶段开发计划
│   ├── SDK编译指南.md
│   ├── RKNN神经网络推理指南.md
│   ├── RKMPI多媒体处理指南.md
│   ├── 硬件参数规格书.md
│   └── ...
│
├── rv1106-feeder/          # C++ 主程序（CMake）
│   ├── CMakeLists.txt      # 构建配置
│   ├── build.sh            # 交叉编译脚本
│   ├── include/            # 头文件（8个模块）
│   │   ├── video_stream.h
│   │   ├── rtsp_server.h
│   │   ├── rknn_detector.h
│   │   ├── reid_module.h
│   │   ├── food_detector.h
│   │   ├── mqtt_client.h
│   │   ├── uart_comm.h
│   │   └── business_logic.h
│   ├── src/                # 源文件（9个）
│   │   ├── main.cpp
│   │   └── ...
│   ├── config/             # 配置文件
│   │   ├── bowl_roi.json   # 碗ROI坐标
│   │   ├── pets.json       # 宠物注册信息
│   │   └── schedule.json   # 定时任务
│   ├── models/             # RKNN 模型文件
│   ├── iq/                 # ISP IQ 参数
│   └── README.md
│
├── pet-feeder-monitor/     # Python 上位机监控
│   ├── main.py
│   ├── mqtt_client.py
│   ├── video_stream.py
│   ├── overlay.py
│   ├── config.py
│   ├── requirements.txt
│   └── 开发文档.md
│
└── tools/                  # 开发与测试工具
    ├── deploy.sh           # 一键部署脚本
    ├── test_rtsp.py        # RTSP 推流测试
    ├── test_mqtt.py        # MQTT 通信测试
    ├── model_conversion/   # 模型转换工具
    │   ├── convert_yolov5n.py
    │   ├── convert_reid.py
    │   ├── convert_mobilenet.py
    │   └── README.md
    └── README.md
```

## 硬件平台

| 项目 | 规格 |
|------|------|
| 开发板 | Luckfox Pico Zero |
| SoC | Rockchip RV1106 |
| NPU | 0.5 TOPS |
| 摄像头 | MIPI CSI |
| 系统 | Buildroot Linux |

## 功能特性

- 🎯 基于 RKNN NPU 的本地推理（宠物检测）
- 📹 RTSP/MJPEG 实时视频流
- 📡 MQTT 远程控制与状态上报
- 🐾 自动定时喂食 + 行为触发喂食
- 🖥️ Python 上位机监控面板

## 快速开始

### 1. 环境准备

**硬件：**
- Luckfox Pico Zero 开发板
- GC2093 摄像头
- 电源适配器

**软件：**
- Luckfox Pico SDK（[下载地址](https://github.com/LuckfoxTECH/luckfox-pico)）
- Python 3.8+（用于模型转换和上位机）

### 2. 编译 C++ 主程序

```bash
# 设置 SDK 路径
export SDK_PATH=/path/to/luckfox-pico

# 交叉编译
cd rv1106-feeder
./build.sh
```

### 3. 转换模型

```bash
cd tools/model_conversion

# 安装依赖
pip install -r requirements.txt

# 转换 YOLOv5n
python3 convert_yolov5n.py \
    --weight yolov5n.pt \
    --output ../../rv1106-feeder/models/yolov5n.rknn

# 转换 Re-ID 模型
python3 convert_reid.py \
    --model osnet_x0_25.pth \
    --output ../../rv1106-feeder/models/reid.rknn

# 转换余量检测模型（需自己训练）
python3 convert_mobilenet.py \
    --model food_classifier.pth \
    --output ../../rv1106-feeder/models/mobilenet.rknn
```

### 4. 部署到设备

```bash
# 一键部署（设备IP: 192.168.1.100）
./tools/deploy.sh 192.168.1.100
```

### 5. 运行程序

```bash
# SSH 登录设备
ssh root@192.168.1.100

# 运行主程序
cd /userdata/bin
./pet_feeder
```

### 6. 测试功能

**测试 RTSP 推流：**
```bash
python3 tools/test_rtsp.py --url rtsp://192.168.1.100:554/live/0
```

**测试 MQTT 通信：**
```bash
python3 tools/test_mqtt.py --broker 192.168.1.100 --publish
```

**运行 Python 上位机：**
```bash
cd pet-feeder-monitor
pip install -r requirements.txt
python main.py
```

## 开发进度

- ✅ 项目架构设计
- ✅ C++ 核心框架（8个模块，17个文件）
- ✅ 模型转换工具（3个转换脚本）
- ✅ 测试与部署工具
- ✅ 完整文档（14篇开发文档）
- ⬜ RKMPI 实际调用实现（待板端调试）
- ⬜ RKNN 模型实际推理（待模型转换）
- ⬜ 硬件集成测试（待样机）

当前状态：**框架完成，等待板端实测**

## 文档索引

### 核心文档
- [宠物喂食器技术方案](docs/宠物喂食器技术方案.md) — 完整技术架构与实现细节
- [开发任务计划](docs/开发任务计划.md) — 7阶段开发计划（20-30天）
- [硬件参数规格书](docs/硬件参数规格书.md) — RV1106 硬件规格

### 开发指南
- [SDK编译指南](docs/SDK编译指南.md)
- [RKNN神经网络推理指南](docs/RKNN神经网络推理指南.md)
- [RKMPI多媒体处理指南](docs/RKMPI多媒体处理指南.md)
- [opencv-mobile使用指南](docs/opencv-mobile使用指南.md)

### 模块文档
- [rv1106-feeder README](rv1106-feeder/README.md) — C++ 主程序说明
- [pet-feeder-monitor 开发文档](pet-feeder-monitor/开发文档.md) — Python 上位机
- [tools README](tools/README.md) — 工具使用指南
- [model_conversion README](tools/model_conversion/README.md) — 模型转换指南

### 运维文档
- [登录与文件传输](docs/登录与文件传输.md)
- [NFS挂载配置](docs/NFS挂载配置.md)
- [镜像烧录指南](docs/镜像烧录指南.md)
- [常见问题FAQ](docs/常见问题FAQ.md)

## 技术栈

**硬件平台：**
- SoC: Rockchip RV1106（ARM Cortex-A7 @ 1.2GHz）
- NPU: 0.5 TOPS（支持 INT8/INT16）
- 内存: 256MB DDR3L
- 摄像头: GC2093（200万像素，1080P）

**软件栈：**
- 系统: Buildroot Linux
- 视频: RKMPI（VI/VENC/VPSS）
- 推理: RKNN Runtime
- 网络: RTSP（live555）/ MQTT（paho-mqtt）
- 通信: UART（RV1106 ↔ STM32）

**AI 模型：**
- 宠物检测: YOLOv5n（416×416，13fps）
- 个体识别: OSNet-x0.25（128×128）
- 余量检测: MobileNetV2（128×128）

## 贡献指南

欢迎提交 Issue 和 Pull Request！

**开发流程：**
1. Fork 本仓库
2. 创建功能分支：`git checkout -b feature/your-feature`
3. 提交更改：`git commit -m "feat: 描述"`
4. 推送分支：`git push origin feature/your-feature`
5. 提交 Pull Request

## 常见问题

### Q1: 编译失败

确认已正确安装 Luckfox Pico SDK 并设置环境变量：
```bash
export SDK_PATH=/path/to/luckfox-pico
```

### Q2: RTSP 推流无法连接

- 检查设备是否运行 `pet_feeder` 程序
- 确认端口 554 未被占用
- 尝试：`telnet 192.168.1.100 554`

### Q3: 模型推理失败

- 确认模型文件已上传到 `/userdata/models/`
- 检查 RKNN Runtime 版本与 Toolkit 版本是否匹配
- 查看日志：`dmesg | grep rknn`

### Q4: 余量检测模型在哪？

余量检测模型需要样机到货后采集数据训练，参考 [model_conversion README](tools/model_conversion/README.md) 中的训练脚本。

## 致谢

- [Luckfox Pico](https://github.com/LuckfoxTECH/luckfox-pico) — 开发板SDK
- [YOLOv5](https://github.com/ultralytics/yolov5) — 目标检测
- [RKNN Toolkit](https://github.com/rockchip-linux/rknn-toolkit2) — 模型转换

## License

MIT
