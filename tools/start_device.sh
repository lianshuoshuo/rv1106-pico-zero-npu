#!/bin/bash
# 快速启动脚本 - 在设备上运行 pet_feeder

WORK_DIR="/userdata"
BIN_PATH="${WORK_DIR}/bin/pet_feeder"
LOG_DIR="${WORK_DIR}/logs"
LOG_FILE="${LOG_DIR}/pet_feeder_$(date +%Y%m%d_%H%M%S).log"

# 创建日志目录
mkdir -p "$LOG_DIR"

# 检查可执行文件
if [ ! -f "$BIN_PATH" ]; then
    echo "错误: 找不到可执行文件 $BIN_PATH"
    exit 1
fi

# 检查是否已经在运行
if pgrep -x "pet_feeder" > /dev/null; then
    echo "警告: pet_feeder 已经在运行"
    echo "进程列表:"
    ps | grep pet_feeder | grep -v grep
    echo ""
    read -p "是否停止现有进程并重新启动? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "停止现有进程..."
        killall pet_feeder
        sleep 2
    else
        exit 0
    fi
fi

echo "=========================================="
echo "  启动 RV1106 宠物喂食器"
echo "=========================================="
echo "工作目录: $WORK_DIR"
echo "可执行文件: $BIN_PATH"
echo "日志文件: $LOG_FILE"
echo ""

# 切换到工作目录
cd "$WORK_DIR" || exit 1

# 启动程序（后台运行）
echo "启动程序..."
nohup "$BIN_PATH" > "$LOG_FILE" 2>&1 &
PID=$!

sleep 2

# 检查进程是否启动成功
if ps -p $PID > /dev/null; then
    echo "✓ 程序启动成功！"
    echo "  PID: $PID"
    echo ""
    echo "查看日志: tail -f $LOG_FILE"
    echo "停止程序: kill $PID"
    echo ""
else
    echo "✗ 程序启动失败"
    echo "查看日志:"
    tail -20 "$LOG_FILE"
    exit 1
fi
