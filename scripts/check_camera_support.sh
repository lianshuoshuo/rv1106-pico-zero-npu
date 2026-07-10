#!/bin/bash

# GC2093 摄像头支持检查脚本
# 用法：将此脚本传输到开发板并执行
# 或通过 SSH 远程执行：ssh root@<板子IP> 'bash -s' < check_camera_support.sh

echo "======================================"
echo "  RV1106 摄像头支持检查工具"
echo "======================================"
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 1. 检查内核版本和系统信息
echo "【系统信息】"
echo "内核版本: $(uname -r)"
echo "系统版本: $(cat /etc/issue 2>/dev/null | head -n 1 || echo '未知')"
echo ""

# 2. 检查已加载的摄像头驱动模块
echo "【已加载的摄像头驱动】"
loaded_drivers=$(lsmod 2>/dev/null | grep -iE "gc2093|gc2053|sc2336|ov5647" | awk '{print $1}' || echo "")
if [ -z "$loaded_drivers" ]; then
    echo -e "${YELLOW}⚠ 未检测到已加载的摄像头驱动模块${NC}"
else
    echo -e "${GREEN}✓ 已加载驱动:${NC}"
    echo "$loaded_drivers"
fi
echo ""

# 3. 检查可用的摄像头驱动模块文件
echo "【可用的摄像头驱动模块】"
module_path="/lib/modules/$(uname -r)/kernel/drivers/media/i2c"
if [ -d "$module_path" ]; then
    echo "驱动目录: $module_path"
    camera_modules=$(ls "$module_path" 2>/dev/null | grep -iE "gc2093|gc2053|sc2336|ov5647" || echo "")
    if [ -z "$camera_modules" ]; then
        echo -e "${RED}✗ 未找到常见摄像头驱动模块${NC}"
    else
        echo -e "${GREEN}✓ 可用驱动模块:${NC}"
        echo "$camera_modules" | while read module; do
            echo "  - $module"
        done

        # 检查是否有 GC2093
        if echo "$camera_modules" | grep -q "gc2093"; then
            echo -e "${GREEN}✓ GC2093 驱动模块存在${NC}"
        else
            echo -e "${RED}✗ GC2093 驱动模块不存在${NC}"
        fi
    fi
else
    echo -e "${RED}✗ 驱动目录不存在: $module_path${NC}"
fi
echo ""

# 4. 检查内核日志中的摄像头信息
echo "【内核日志中的摄像头信息】"
camera_log=$(dmesg | grep -iE "gc2093|gc2053|sc2336|camera|csi|mipi" | tail -n 10)
if [ -z "$camera_log" ]; then
    echo -e "${YELLOW}⚠ 未找到摄像头相关日志${NC}"
else
    echo "$camera_log"
fi
echo ""

# 5. 检查 I2C 设备
echo "【I2C 总线扫描】"
i2c_buses=$(ls /dev/i2c-* 2>/dev/null)
if [ -z "$i2c_buses" ]; then
    echo -e "${RED}✗ 未找到 I2C 设备${NC}"
else
    echo "检测到的 I2C 总线:"
    echo "$i2c_buses"
    echo ""

    # 扫描 I2C-3（通常是摄像头总线）
    if [ -e "/dev/i2c-3" ]; then
        echo "扫描 I2C-3 总线（摄像头常用）:"
        i2cdetect -y 3 2>/dev/null || echo -e "${YELLOW}⚠ i2cdetect 命令不可用${NC}"
        echo ""
        echo "常见摄像头 I2C 地址："
        echo "  - GC2093: 0x37"
        echo "  - GC2053: 0x37"
        echo "  - SC2336: 0x30"
    else
        echo -e "${YELLOW}⚠ /dev/i2c-3 不存在${NC}"
    fi
fi
echo ""

# 6. 检查视频设备节点
echo "【视频设备节点】"
video_devices=$(ls /dev/video* 2>/dev/null)
if [ -z "$video_devices" ]; then
    echo -e "${RED}✗ 未找到视频设备节点${NC}"
else
    echo -e "${GREEN}✓ 视频设备:${NC}"
    for dev in $video_devices; do
        echo "  - $dev"
        # 尝试获取设备信息
        v4l2-ctl --device=$dev --info 2>/dev/null | grep -E "Card type|Driver name" | sed 's/^/    /'
    done
fi
echo ""

# 7. 检查 RKIPC 配置
echo "【RKIPC 配置】"
if [ -f "/userdata/rkipc.ini" ]; then
    echo -e "${GREEN}✓ RKIPC 配置文件存在${NC}"
    sensor_name=$(grep "sensor_name" /userdata/rkipc.ini 2>/dev/null | head -n 1)
    iq_file=$(grep "iq_file" /userdata/rkipc.ini 2>/dev/null | head -n 1)

    if [ -n "$sensor_name" ]; then
        echo "  $sensor_name"
        current_sensor=$(echo "$sensor_name" | cut -d'=' -f2 | tr -d ' ')

        if [ "$current_sensor" = "gc2093" ]; then
            echo -e "  ${GREEN}✓ 当前使用 GC2093${NC}"
        else
            echo -e "  ${YELLOW}⚠ 当前使用: $current_sensor (非 GC2093)${NC}"
        fi
    fi

    if [ -n "$iq_file" ]; then
        echo "  $iq_file"
    fi
else
    echo -e "${YELLOW}⚠ RKIPC 配置文件不存在${NC}"
    echo "  可能原因："
    echo "  1. 首次启动，未自动生成配置"
    echo "  2. 摄像头未连接或未识别"
fi
echo ""

# 8. 检查 IQ 文件
echo "【IQ 文件检查】"
iq_dir="/etc/iqfiles"
if [ -d "$iq_dir" ]; then
    echo "IQ 文件目录: $iq_dir"
    iq_files=$(ls "$iq_dir"/*.json 2>/dev/null)
    if [ -z "$iq_files" ]; then
        echo -e "${YELLOW}⚠ 未找到 IQ 文件${NC}"
    else
        echo -e "${GREEN}✓ 可用的 IQ 文件:${NC}"
        for iq in $iq_files; do
            size=$(ls -lh "$iq" | awk '{print $5}')
            echo "  - $(basename $iq) ($size)"
        done

        # 检查是否有 GC2093 IQ 文件
        if ls "$iq_dir"/gc2093*.json >/dev/null 2>&1; then
            echo -e "${GREEN}✓ GC2093 IQ 文件存在${NC}"
        else
            echo -e "${RED}✗ GC2093 IQ 文件不存在${NC}"
        fi
    fi
else
    echo -e "${RED}✗ IQ 文件目录不存在: $iq_dir${NC}"
fi
echo ""

# 9. 检查设备树信息
echo "【设备树摄像头配置】"
if [ -d "/proc/device-tree" ]; then
    # 查找摄像头相关节点
    camera_nodes=$(find /proc/device-tree -name "*gc2093*" -o -name "*camera*" 2>/dev/null | head -n 5)
    if [ -z "$camera_nodes" ]; then
        echo -e "${YELLOW}⚠ 未找到摄像头设备树节点${NC}"
    else
        echo "找到的摄像头相关节点:"
        echo "$camera_nodes"
    fi
else
    echo -e "${YELLOW}⚠ 无法访问设备树信息${NC}"
fi
echo ""

# 10. 总结
echo "======================================"
echo "  检查结果总结"
echo "======================================"
echo ""

# 判断是否支持 GC2093
gc2093_module_exists=false
gc2093_iq_exists=false
gc2093_in_use=false

# 检查模块
if [ -f "$module_path/gc2093.ko" ]; then
    gc2093_module_exists=true
fi

# 检查 IQ 文件
if ls /etc/iqfiles/gc2093*.json >/dev/null 2>&1; then
    gc2093_iq_exists=true
fi

# 检查是否正在使用
if [ -f "/userdata/rkipc.ini" ]; then
    current_sensor=$(grep "sensor_name" /userdata/rkipc.ini 2>/dev/null | cut -d'=' -f2 | tr -d ' ')
    if [ "$current_sensor" = "gc2093" ]; then
        gc2093_in_use=true
    fi
fi

echo "GC2093 支持状态："
if $gc2093_module_exists; then
    echo -e "  ${GREEN}✓ 驱动模块存在${NC}"
else
    echo -e "  ${RED}✗ 驱动模块缺失${NC}"
fi

if $gc2093_iq_exists; then
    echo -e "  ${GREEN}✓ IQ 文件存在${NC}"
else
    echo -e "  ${RED}✗ IQ 文件缺失${NC}"
fi

if $gc2093_in_use; then
    echo -e "  ${GREEN}✓ 当前正在使用 GC2093${NC}"
else
    echo -e "  ${YELLOW}⚠ 当前未使用 GC2093${NC}"
    if [ -n "$current_sensor" ]; then
        echo -e "  当前使用的传感器: ${YELLOW}$current_sensor${NC}"
    fi
fi

echo ""

if $gc2093_module_exists && $gc2093_iq_exists; then
    echo -e "${GREEN}✓ 当前固件支持 GC2093${NC}"
    if ! $gc2093_in_use; then
        echo ""
        echo "建议操作："
        echo "1. 连接 GC2093 摄像头模组"
        echo "2. 删除旧配置: rm /userdata/rkipc.ini"
        echo "3. 重启开发板让系统自动识别"
        echo "4. 或手动配置 /userdata/rkipc.ini 使用 gc2093"
    fi
elif $gc2093_module_exists && ! $gc2093_iq_exists; then
    echo -e "${YELLOW}⚠ 固件包含 GC2093 驱动，但缺少 IQ 文件${NC}"
    echo ""
    echo "需要操作："
    echo "1. 下载 gc2093.json 到 /etc/iqfiles/ 目录"
    echo "2. 连接 GC2093 摄像头模组"
    echo "3. 重启开发板"
else
    echo -e "${RED}✗ 当前固件不支持 GC2093${NC}"
    echo ""
    echo "需要操作："
    echo "1. 重新烧录包含 GC2093 驱动的固件，或"
    echo "2. 自行编译 SDK 并启用 GC2093 驱动"
fi

echo ""
echo "======================================"
