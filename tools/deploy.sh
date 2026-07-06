#!/bin/bash
# RV1106 宠物喂食器一键部署脚本

set -e

DEVICE_IP="${1:-192.168.1.100}"
DEVICE_USER="root"
DEVICE_BASE="/userdata"

echo "=========================================="
echo "  RV1106 宠物喂食器部署脚本"
echo "=========================================="
echo "目标设备: $DEVICE_USER@$DEVICE_IP"
echo "部署路径: $DEVICE_BASE"
echo ""

# 检查设备连接
echo "[1/6] 检查设备连接..."
if ! ping -c 1 -W 2 "$DEVICE_IP" > /dev/null 2>&1; then
    echo "✗ 设备无法访问: $DEVICE_IP"
    echo "请检查网络连接或修改设备IP"
    exit 1
fi
echo "✓ 设备连接正常"

# 检查可执行文件
echo ""
echo "[2/6] 检查本地文件..."
if [ ! -f "rv1106-feeder/build/pet_feeder" ]; then
    echo "✗ 可执行文件不存在: rv1106-feeder/build/pet_feeder"
    echo "请先运行: cd rv1106-feeder && ./build.sh"
    exit 1
fi
echo "✓ 可执行文件存在"

# 创建远程目录
echo ""
echo "[3/6] 创建远程目录..."
ssh "$DEVICE_USER@$DEVICE_IP" "mkdir -p $DEVICE_BASE/{bin,config,models,iq,logs}" || {
    echo "✗ 创建目录失败"
    exit 1
}
echo "✓ 目录创建成功"

# 上传可执行文件
echo ""
echo "[4/6] 上传可执行文件..."
scp rv1106-feeder/build/pet_feeder "$DEVICE_USER@$DEVICE_IP:$DEVICE_BASE/bin/" || {
    echo "✗ 上传失败"
    exit 1
}
ssh "$DEVICE_USER@$DEVICE_IP" "chmod +x $DEVICE_BASE/bin/pet_feeder"
echo "✓ 可执行文件上传成功"

# 上传配置文件
echo ""
echo "[5/6] 上传配置文件..."
scp rv1106-feeder/config/*.json "$DEVICE_USER@$DEVICE_IP:$DEVICE_BASE/config/" 2>/dev/null || {
    echo "⊙ 配置文件上传跳过（文件不存在）"
}
echo "✓ 配置文件上传完成"

# 上传模型文件（可选）
echo ""
echo "[6/6] 上传模型文件..."
if [ -d "rv1106-feeder/models" ] && [ "$(ls -A rv1106-feeder/models/*.rknn 2>/dev/null)" ]; then
    scp rv1106-feeder/models/*.rknn "$DEVICE_USER@$DEVICE_IP:$DEVICE_BASE/models/" || {
        echo "⊙ 模型文件上传失败"
    }
    echo "✓ 模型文件上传成功"
else
    echo "⊙ 模型文件不存在，跳过上传"
    echo "提示: 请使用 tools/model_conversion/ 转换模型"
fi

echo ""
echo "=========================================="
echo "✓ 部署完成！"
echo "=========================================="
echo ""
echo "运行程序："
echo "  ssh $DEVICE_USER@$DEVICE_IP"
echo "  cd $DEVICE_BASE/bin"
echo "  ./pet_feeder"
echo ""
echo "查看日志："
echo "  tail -f $DEVICE_BASE/logs/pet_feeder.log"
echo ""
echo "测试 RTSP 推流："
echo "  vlc rtsp://$DEVICE_IP:554/live/0"
echo ""
