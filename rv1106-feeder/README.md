# RV1106 Feeder — C++ 主程序

基于 RV1106 的智能宠物喂食器核心程序，集成视频推流、NPU推理、MQTT通信与喂食控制。

## 功能特性

- ✅ **视频推流**：1080P@25fps RTSP 推流（H.264 CBR）
- ✅ **宠物检测**：YOLOv5n RKNN 推理（13fps+）
- ✅ **个体识别**：Re-ID 多宠物识别
- ✅ **余量检测**：MobileNetV2 视觉余量分类
- ✅ **MQTT通信**：状态上报与远程控制
- ✅ **UART控制**：RV1106 ↔ STM32 喂食指令
- ✅ **业务逻辑**：三种喂食触发（APP/定时/行为）

## 目录结构

```
rv1106-feeder/
├── CMakeLists.txt          # CMake 构建配置
├── build.sh                # 交叉编译脚本
├── include/                # 头文件
│   ├── video_stream.h
│   ├── rtsp_server.h
│   ├── rknn_detector.h
│   ├── reid_module.h
│   ├── food_detector.h
│   ├── mqtt_client.h
│   ├── uart_comm.h
│   └── business_logic.h
├── src/                    # 源文件
│   ├── main.cpp
│   ├── video_stream.cpp
│   ├── rtsp_server.cpp
│   ├── rknn_detector.cpp
│   ├── reid_module.cpp
│   ├── food_detector.cpp
│   ├── mqtt_client.cpp
│   ├── uart_comm.cpp
│   └── business_logic.cpp
├── config/                 # 配置文件
│   ├── bowl_roi.json       # 碗ROI坐标
│   ├── pets.json           # 宠物注册信息
│   └── schedule.json       # 定时任务
├── models/                 # RKNN 模型文件
│   ├── yolov5n.rknn
│   ├── reid.rknn
│   └── mobilenet.rknn
├── iq/                     # ISP IQ 参数
└── third_party/            # 第三方库
```

## 编译

### 前置条件

1. **Luckfox Pico SDK**
   ```bash
   git clone https://github.com/LuckfoxTECH/luckfox-pico.git
   cd luckfox-pico
   ./build.sh  # 构建SDK
   ```

2. **环境变量**
   ```bash
   export SDK_PATH=/path/to/luckfox-pico
   ```

### 交叉编译

```bash
cd rv1106-feeder
./build.sh
```

编译成功后生成：`build/pet_feeder`

## 部署

### 1. 上传程序到设备

```bash
# 上传可执行文件
scp build/pet_feeder root@192.168.1.100:/userdata/bin/

# 上传配置文件
scp -r config/* root@192.168.1.100:/userdata/config/

# 上传模型文件（需提前转换为 .rknn）
scp -r models/* root@192.168.1.100:/userdata/models/
```

### 2. 设置权限

```bash
ssh root@192.168.1.100
chmod +x /userdata/bin/pet_feeder
```

### 3. 运行

```bash
# SSH登录设备
ssh root@192.168.1.100

# 运行程序
cd /userdata/bin
./pet_feeder
```

## 配置文件说明

### bowl_roi.json — 碗ROI坐标

```json
{
  "bowl": {
    "x": 320,
    "y": 400,
    "w": 180,
    "h": 160
  }
}
```

### pets.json — 宠物注册信息

```json
{
  "pets": [
    {
      "id": "cat_001",
      "name": "小花",
      "feature": [0.12, 0.34, ..., 0.56]
    },
    {
      "id": "cat_002",
      "name": "小白",
      "feature": [0.78, 0.23, ..., 0.91]
    }
  ]
}
```

### schedule.json — 定时任务

```json
{
  "schedules": [
    {"hour": 8, "minute": 0, "enabled": 1},
    {"hour": 18, "minute": 0, "enabled": 1}
  ]
}
```

## RTSP 推流地址

启动后可通过以下地址拉流：

```
rtsp://<设备IP>:554/live/0
```

VLC 播放器测试：
```bash
vlc rtsp://192.168.1.100:554/live/0
```

## 依赖库

编译时需要链接以下库（SDK 中提供）：

- **RKMPI**：`rockchip_mpi`, `rkaiq`, `easymedia`
- **RKNN**：`rknnrt`
- **MQTT**：`paho-mqtt3c`
- **SQLite**：`sqlite3`
- **OpenCV**（可选）：`opencv_core`, `opencv_imgproc`

## 开发说明

### 代码框架

当前代码提供完整的框架和接口定义，实际的 RKMPI/RKNN 调用已用注释标注 `TODO`，需要根据实际SDK完善。

### 待完善模块

1. **video_stream.cpp**：RKMPI VI/VENC 实际调用
2. **rtsp_server.cpp**：集成 RTSP 库（live555 或 RK 官方）
3. **rknn_detector.cpp**：RKNN 推理完整流程
4. **reid_module.cpp**：Re-ID 模型推理
5. **food_detector.cpp**：余量检测模型推理
6. **mqtt_client.cpp**：paho-mqtt 实际调用

### 模型转换

#### YOLOv5n

```bash
# PC端转换（需要 RKNN Toolkit 2）
python export.py --rknpu --weight yolov5n.pt --img 416
python convert_rknn.py --model yolov5n.onnx --platform rv1106 --output yolov5n.rknn
```

#### Re-ID (OSNet-x0.25)

```bash
python convert_reid.py --model osnet_x0_25.pth --output reid.rknn
```

#### MobileNetV2

```bash
python convert_food_classifier.py --model mobilenetv2.pth --output mobilenet.rknn
```

## 性能指标

| 指标 | 目标值 | 说明 |
|------|--------|------|
| 推流帧率 | 25fps | 稳定不丢帧 |
| 推流分辨率 | 1080P | 1920×1080 |
| 推理帧率 | 13fps+ | YOLOv5n @ 416×416 |
| 内存占用 | <160MB | 包含所有模块 |
| 推流延迟 | <200ms | 局域网环境 |

## 问题排查

### 推流无法连接

```bash
# 检查 RTSP 端口
netstat -tulnp | grep 554

# 检查防火墙
iptables -L
```

### 模型加载失败

```bash
# 检查模型文件
ls -lh /userdata/models/

# 查看日志
dmesg | grep rknn
```

### UART 通信失败

```bash
# 检查串口设备
ls -l /dev/ttyS*

# 测试串口
echo "test" > /dev/ttyS1
```

## 相关文档

- [宠物喂食器技术方案](../docs/宠物喂食器技术方案.md)
- [RKNN神经网络推理指南](../docs/RKNN神经网络推理指南.md)
- [RKMPI多媒体处理指南](../docs/RKMPI多媒体处理指南.md)

## License

MIT
