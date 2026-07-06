#!/bin/bash
# RV1106 宠物喂食器 - 系统服务安装脚本

set -e

SERVICE_NAME="pet-feeder"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"
EXEC_PATH="/userdata/bin/pet_feeder"
WORK_DIR="/userdata"

echo "=========================================="
echo "  安装 pet-feeder 系统服务"
echo "=========================================="

# 检查是否为 root
if [ "$(id -u)" -ne 0 ]; then
    echo "✗ 请使用 root 权限运行此脚本"
    echo "  sudo $0"
    exit 1
fi

# 检查可执行文件是否存在
if [ ! -f "$EXEC_PATH" ]; then
    echo "✗ 可执行文件不存在: $EXEC_PATH"
    echo "  请先部署程序到设备"
    exit 1
fi

# 创建 systemd 服务文件
echo "创建服务配置..."
cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=RV1106 Pet Feeder Service
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=$WORK_DIR
ExecStart=$EXEC_PATH
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

echo "✓ 服务配置已创建: $SERVICE_FILE"

# 重载 systemd 配置
echo "重载 systemd 配置..."
systemctl daemon-reload

# 启用服务（开机自启）
echo "启用服务..."
systemctl enable $SERVICE_NAME

# 启动服务
echo "启动服务..."
systemctl start $SERVICE_NAME

# 检查服务状态
sleep 2
echo ""
echo "=========================================="
echo "服务状态:"
echo "=========================================="
systemctl status $SERVICE_NAME --no-pager

echo ""
echo "=========================================="
echo "✓ 安装完成！"
echo "=========================================="
echo ""
echo "常用命令:"
echo "  启动服务: systemctl start $SERVICE_NAME"
echo "  停止服务: systemctl stop $SERVICE_NAME"
echo "  重启服务: systemctl restart $SERVICE_NAME"
echo "  查看状态: systemctl status $SERVICE_NAME"
echo "  查看日志: journalctl -u $SERVICE_NAME -f"
echo "  禁用自启: systemctl disable $SERVICE_NAME"
echo ""
