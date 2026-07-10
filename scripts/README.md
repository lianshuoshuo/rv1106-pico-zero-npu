# 脚本使用说明

## check_camera_support.sh

检查 RV1106 开发板当前固件支持的摄像头型号，特别是 GC2093 的支持情况。

### 使用方法

#### 方法 1：SSH 远程执行（推荐）

```bash
# 通过 SSH 远程执行脚本
ssh root@172.32.0.93 'bash -s' < scripts/check_camera_support.sh
```

#### 方法 2：传输到开发板执行

```bash
# 使用 SCP 传输
scp scripts/check_camera_support.sh root@172.32.0.93:/tmp/

# SSH 登录并执行
ssh root@172.32.0.93
cd /tmp
chmod +x check_camera_support.sh
./check_camera_support.sh
```

#### 方法 3：使用 ADB（如果已安装）

```bash
# 传输脚本
adb push scripts/check_camera_support.sh /tmp/

# 执行脚本
adb shell "chmod +x /tmp/check_camera_support.sh && /tmp/check_camera_support.sh"
```

### 检查内容

脚本会检查以下信息：

1. **系统信息**：内核版本、系统版本
2. **已加载的驱动**：当前运行的摄像头驱动模块
3. **可用的驱动模块**：固件中包含的摄像头驱动文件
4. **内核日志**：摄像头初始化相关的日志
5. **I2C 总线**：扫描摄像头设备（地址 0x37 为 GC2093）
6. **视频设备**：/dev/video* 设备节点
7. **RKIPC 配置**：当前使用的摄像头型号
8. **IQ 文件**：已部署的图像质量参数文件
9. **设备树**：摄像头硬件配置信息
10. **总结报告**：GC2093 支持状态和操作建议

### 输出示例

```
======================================
  RV1106 摄像头支持检查工具
======================================

【系统信息】
内核版本: 5.10.160
系统版本: Buildroot 2023.02

【已加载的摄像头驱动】
✓ 已加载驱动:
gc2093

【可用的摄像头驱动模块】
驱动目录: /lib/modules/5.10.160/kernel/drivers/media/i2c
✓ 可用驱动模块:
  - gc2093.ko
  - gc2053.ko
✓ GC2093 驱动模块存在

...

======================================
  检查结果总结
======================================

GC2093 支持状态：
  ✓ 驱动模块存在
  ✓ IQ 文件存在
  ⚠ 当前未使用 GC2093
  当前使用的传感器: gc2093

✓ 当前固件支持 GC2093

建议操作：
1. 连接 GC2093 摄像头模组
2. 删除旧配置: rm /userdata/rkipc.ini
3. 重启开发板让系统自动识别
4. 或手动配置 /userdata/rkipc.ini 使用 gc2093
```

### 常见结果解读

#### ✓ 固件支持 GC2093
- 驱动模块存在：`/lib/modules/.../gc2093.ko`
- IQ 文件存在：`/etc/iqfiles/gc2093.json`
- **可以直接使用 GC2093 摄像头**

#### ⚠ 驱动存在但缺少 IQ 文件
- 需要下载 gc2093.json 并传输到开发板
- 参考：[GC2093驱动与IQ获取指南.md](../docs/GC2093驱动与IQ获取指南.md)

#### ✗ 固件不支持 GC2093
- 需要重新烧录固件或自行编译 SDK
- 参考：[SDK编译指南.md](../docs/SDK编译指南.md)

### 网络连接说明

开发板连接方式（任选其一）：

1. **USB 连接（静态 IP）**
   - Buildroot: `172.32.0.93`
   - Ubuntu: `172.32.0.70`

2. **WiFi 连接**
   - 配置 WiFi 后获取动态 IP
   - 参考：[外设与接口.md](../docs/外设与接口.md)

3. **以太网连接**
   - 通过路由器 DHCP 获取 IP

### 故障排查

**无法连接开发板？**
```bash
# 检查网络连通性
ping 172.32.0.93

# 检查 SSH 服务
telnet 172.32.0.93 22

# macOS 配置 USB 网卡
sudo ifconfig bridge100 172.32.0.100 netmask 255.255.255.0
```

**权限被拒绝？**
```bash
# 使用正确的用户名和密码
# Buildroot: root / luckfox
# Ubuntu: pico / luckfox

ssh root@172.32.0.93
# 输入密码: luckfox
```

### 相关文档

- [登录与文件传输.md](../docs/登录与文件传输.md)
- [外设与接口.md](../docs/外设与接口.md)
- [GC2093完整配置指南.md](../docs/GC2093完整配置指南.md)
