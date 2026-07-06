#!/bin/bash
# RV1106 交叉编译脚本

set -e

# 配置SDK路径（根据实际修改）
SDK_PATH="${SDK_PATH:-$HOME/luckfox-pico}"
TOOLCHAIN_FILE="${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf.cmake"

# 检查SDK路径
if [ ! -d "$SDK_PATH" ]; then
    echo "错误: SDK路径不存在: $SDK_PATH"
    echo "请设置环境变量: export SDK_PATH=/path/to/luckfox-pico"
    exit 1
fi

# 创建构建目录
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "清理旧的构建目录..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# CMake配置
echo "CMake 配置中..."
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/userdata

# 编译
echo "开始编译..."
make -j$(nproc)

echo ""
echo "✅ 编译成功！"
echo "可执行文件: $BUILD_DIR/pet_feeder"
echo ""
echo "部署到设备："
echo "  scp pet_feeder root@<设备IP>:/userdata/bin/"
echo "  ssh root@<设备IP> 'chmod +x /userdata/bin/pet_feeder'"
