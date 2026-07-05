# RV1106 Pico Zero NPU — 智能宠物喂食器

基于 Luckfox Pico Zero（RV1106）开发的智能宠物喂食器项目，集成 NPU 推理、视频监控与远程控制。

## 项目结构

```
.
├── docs/                   # 开发文档
│   ├── SDK编译指南.md
│   ├── RKNN神经网络推理指南.md
│   ├── RKMPI多媒体处理指南.md
│   ├── opencv-mobile使用指南.md
│   ├── 宠物喂食器技术方案.md
│   └── ...（共13篇）
├── rv1106-feeder/          # C++ 主程序（CMake）
│   ├── CMakeLists.txt
│   ├── src/
│   ├── include/
│   ├── config/
│   ├── models/             # RKNN 模型文件
│   ├── iq/                 # ISP IQ 参数
│   └── third_party/
└── pet-feeder-monitor/     # Python 上位机监控
    ├── main.py
    ├── mqtt_client.py
    ├── video_stream.py
    ├── overlay.py
    ├── config.py
    └── requirements.txt
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

### 编译 C++ 主程序

```bash
cd rv1106-feeder
# 交叉编译（需要 RV1106 SDK 工具链）
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<sdk>/toolchain.cmake
make -j4
```

### 运行 Python 监控

```bash
cd pet-feeder-monitor
pip install -r requirements.txt
python main.py
```

## 文档

详细开发文档见 [docs/](docs/) 目录。

## License

MIT
