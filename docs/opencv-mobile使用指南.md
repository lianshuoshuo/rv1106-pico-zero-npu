# opencv-mobile 使用指南

> 更新日期：2026-07-04  
> opencv-mobile 版本：4.10.0  
> 平台：Luckfox Pico Zero（RV1106G3）

---

## 一、概述

opencv-mobile 是精简版 OpenCV，体积约为官方版本的 1/10，专为移动和嵌入式环境设计。在 RV1106 上具备以下硬件加速支持：

| 功能 | 加速方式 |
|------|---------|
| 摄像头采集 | v4l2 + rkaiq（ISP 自动调优） |
| YUV → BGR 转换 | RGA 硬件加速 |
| JPEG 编码（imwrite） | rkmpp 硬件加速 |

> 与 RKMPI 方式相比，opencv-mobile 帧率约 ~9fps，RKMPI VI 采集约 ~23fps。opencv-mobile 适合原型验证，正式产品推荐 RKMPI。

---

## 二、获取预编译包

```bash
# 从 GitHub 获取最新版本
# https://github.com/nihui/opencv-mobile/releases
# 下载：opencv-mobile-4.10.0-luckfox-pico.zip

unzip opencv-mobile-4.10.0-luckfox-pico.zip -d ./
```

---

## 三、CMakeLists.txt 配置

```cmake
project(my-app)
cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)

# 指向 SDK 中的交叉编译工具链
set(SDK_DIR "/path/to/luckfox-pico")
set(TOOLCHAIN "${SDK_DIR}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin")

SET(CMAKE_C_COMPILER   "${TOOLCHAIN}/arm-rockchip830-linux-uclibcgnueabihf-gcc")
SET(CMAKE_CXX_COMPILER "${TOOLCHAIN}/arm-rockchip830-linux-uclibcgnueabihf-g++")

# 指向解压后的 opencv-mobile 目录
set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/opencv-mobile-4.10.0-luckfox-pico/lib/cmake/opencv4")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(my-app main.cpp)
target_link_libraries(my-app ${OpenCV_LIBS})
```

---

## 四、编译

```bash
mkdir build && cd build
cmake ..
make -j4
```

---

## 五、使用示例

采集9帧（每秒1帧），拼接为3×3图像网格保存：

```cpp
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <unistd.h>

int main() {
    cv::VideoCapture cap;
    cap.set(cv::CAP_PROP_FRAME_WIDTH,  320);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 240);
    cap.open(0);

    int w = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int h = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    cv::Mat bgr[9];
    for (int i = 0; i < 9; i++) {
        cap >> bgr[i];
        sleep(1);
    }
    cap.release();

    // 拼接 3×3 网格
    cv::Mat out(h * 3, w * 3, CV_8UC3);
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            bgr[r*3+c].copyTo(out(cv::Rect(c*w, r*h, w, h)));

    cv::imwrite("out.jpg", out);
    return 0;
}
```

无需任何额外代码，`cv::VideoCapture` 会自动触发 rkaiq ISP 和 RGA 加速。

---

## 六、部署到开发板

```bash
# 传输可执行文件
scp build/my-app root@172.32.0.93:/root

# 板端：先释放摄像头占用，再运行
ssh root@172.32.0.93
RkLunch-stop.sh   # 或 killall rkipc
./my-app
```

---

## 七、关键技术细节

### 7.1 摄像头设备节点

v4l2 会枚举多个 video 设备，rkaiq ISP 处理后的帧来自 `rkisp_mainpath`：

```bash
# 查找正确的设备节点（通常为 /dev/video11）
ls /sys/class/video4linux/
cat /sys/class/video4linux/video11/name   # 输出：rkisp_mainpath
```

### 7.2 ISP 初始化延迟

打开摄像头后，ISP 需要数秒收集统计信息进行自动曝光/白平衡收敛，**前几帧图像会偏暗**，属正常现象，稳定后自动恢复。

### 7.3 分辨率约束

| 约束 | 说明 |
|------|------|
| 宽度 | 必须是 16 的倍数 |
| 高度 | 必须是 2 的倍数 |
| 不符合时 | 自动向上取整并裁剪，保持宽高比 |

### 7.4 RGA 加速原理

RGA 要求 DMA buffer 作为输入输出，转换完成后再 memcpy 到 `cv::Mat`，对用户代码完全透明。

### 7.5 平台检测

rkaiq/RGA 加速通过读取 `/proc/device-tree/model` 判断是否为 Luckfox Pico 设备后启用，跨平台移植时无需修改代码。
