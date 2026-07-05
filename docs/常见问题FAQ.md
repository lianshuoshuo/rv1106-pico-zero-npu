# 常见问题 FAQ

> 更新日期：2026-07-04  
> 来源：Luckfox Pico RV1106 官方 Wiki FAQ

---

## Q1. 无法识别 MASKROM 设备

SocToolKit 插上设备后无反应，通常是电脑环境问题而非硬件损坏。

**排查顺序：**
1. 确认已安装瑞芯微 DriverAssistant v5.13 驱动（推荐 Win10/11）
2. 关闭杀毒软件再运行烧录工具
3. 更换高品质 Type-C 数据线（低价线可能仅充电，无数据传输）
4. 关闭虚拟机、手机助手等占用 USB 的软件
5. 使用台式机主板后置 USB 接口，避免前置接口供电不足
6. 不要通过 USB HUB 连接，直连电脑

---

## Q2. 镜像烧录失败

| 现象 | 原因 | 解决 |
|------|------|------|
| 识别到设备但 DownloadBin 失败 | 线材质量问题 | 更换高品质 Type-C 线 |
| 识别到设备但 env 下载失败 | 虚拟机弹窗未确认，或使用了 USB HUB | 移除 HUB，直连电脑 |

---

## Q3. 串口乱码问题

调试串口输出 TTL 电平，**必须用 USB 转 TTL 模块，不可用 RS232 模块**。

**排查顺序：**
1. 确认模块类型为 USB 转 TTL（非 RS232）
2. 核查波特率 `115200`、数据位 `8`、停止位 `1`
3. 确认接线：TX ↔ RX，GND 对接
4. 以上均正确仍乱码则更换串口模块

---

## Q4. 内存和存储分配

以 Pico Zero（RV1106G3）为例：
- **总内存**：256MB，可用约 190MB（66MB 分配给摄像头 CMA）
- **eMMC 分区**：env(256KB) | idblock(256KB) | uboot(512KB) | boot(4MB) | oem(30MB) | userdata(10MB) | rootfs(剩余)

> 不使用摄像头时，将 `BoardConfig_IPC/.mk` 中 `RK_BOOTARGS_CMA_SIZE` 从 66MB 改为 1MB，重编译后可释放摄像头内存。

---

## Q5. 各型号实际可用内存

| 型号 | 总内存 | 摄像头占用 | 实际可用 |
|------|--------|-----------|---------|
| Pico / Mini / Plus | 64MB | 24MB | ~35MB |
| Pico Pro / Ultra B / Pi B | 128MB | 66MB | ~60MB |
| Pico Max / Ultra / Pi A / **Zero (G3)** | 256MB | 66MB | **~190MB** |

配置文件：`luckfox-pico/project/cfg/BoardConfig_IPC/.mk`

---

## Q6. 无法使用摄像头

1. 烧录最新 Buildroot 镜像
2. 检查 MIPI 排线是否牢固、方向正确
3. 确保开发板与电脑在同一局域网，能 ping 通、SSH 可登录
4. 执行以下命令检查摄像头 I2C 是否识别：
   ```bash
   i2cdetect -y 4
   ```
5. 确认有 `rkipc` 进程，且 `/userdata` 目录下有摄像头配置文件

---

## Q7. 系统分区大小问题

### 7.1 分区容量不匹配

默认镜像分区固定，不自动扩展。两种修改方式：

**方式一**：修改 SDK `BoardConfig_IPC/.mk` 中的 rootfs 分区大小，重编译烧录。

**方式二**：进入 U-Boot 手动修改（启动时按 `Ctrl+C`）：

```
setenv blkdevparts 'mmcblk1:32K(env),512K@32K(idblock),256K(uboot),32M(boot),512M(oem),256M(userdata),-(rootfs)'
saveenv
reset
```

> `-` 表示占用剩余全部空间。

### 7.2 为何需要 8GB 以上存储

镜像虽只有几百 MB，但系统启动后会按 SDK 分区设定自动 resize，加上软件安装和日志存储，建议使用 8GB 以上。

---

## Q8. USB 无法识别外设

适用于 Ultra 系列和 Pico Pi 系列（带 USB-A 接口）：

- **Pico Pi**：确认硬件拨码开关在 Host 模式，并通过 `luckfox-config` 切换到 Host 模式
- **Pico Ultra**：通过 `luckfox-config` 切换 Host 模式；USB-A 与 USB-C 不能同时使用，须用 POE 或 GPIO 供电

---

## Q9. 其他供电方式

| 型号 | 供电方式 | 电压范围 |
|------|---------|---------|
| Pico / Mini / Plus / Pro / Max / **Zero** | VBUS（Type-C）或 VSYS | 4.5V ~ 5.5V |
| Pico Ultra 系列 | GPIO 引脚 或 POE | 参考原理图 |

---

## Q10. RGB 屏幕分辨率不正常

默认镜像适配 720×720 屏幕；若屏幕为 480×480，开机后按一下 Boot 键切换分辨率。

手动测试命令：

```bash
modetest -M rockchip -s 70@66:480x480
modetest -M rockchip -s 70@66:720x720
```

---

## Q11. 修改开机 LOGO

```bash
# 安装转换工具
sudo apt-get install netpbm

# 转换图片格式（300px 宽，224 色）
pngtopnm logo.png | pnmscale -width 300 | pnmquant 224 | pnmtoplainpnm > logo_linux_clut224.ppm

# 替换内核 LOGO
cp logo_linux_clut224.ppm sysdrv/source/kernel/drivers/video/logo/

# 重新编译并烧录内核
./build.sh kernel
```

---

## Q12. 修改登录密码

1. 查看当前默认密码哈希：
   ```bash
   openssl passwd -1 -salt dXmV8ZLO -table luckfox
   ```

2. 生成新密码哈希（以 `newpassword` 为例）：
   ```bash
   openssl passwd -1 -salt dXmV8ZLO -table newpassword
   ```

3. 将生成的哈希值替换到：
   `project/cfg/BoardConfig_IPC/overlay/.../etc/shadow`

4. 重新打包：
   ```bash
   ./build.sh firmware
   ```
