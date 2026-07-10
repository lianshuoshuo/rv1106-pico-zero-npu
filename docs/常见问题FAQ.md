# 常见问题 FAQ

> 更新日期：2026-07-10  
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

---

## Q13. 在固件 250802 上集成 GC2093 摄像头

在固件 250802 上集成 GC2093 摄像头涉及以下几个步骤，每步均有坑点。

### 13.1 gc2093.ko 编译问题

**坑1：Ubuntu 22.04 编译内核模块报错 `python: not found`**

内核构建脚本调用 `python`（非 `python3`），Ubuntu 22.04 默认没有该命令。

```dockerfile
# Dockerfile 中必须加这一行
RUN apt-get install -y python-is-python3
```

**坑2：vermagic 不匹配导致 insmod 失败**

编译时必须使用与板上固件相同的 `luckfox_rv1106_linux_defconfig`，不能用任意 defconfig。可用标准 Ubuntu apt 安装的 `arm-linux-gnueabihf-gcc`，不需要 Rockchip 私有 toolchain。

---

### 13.2 boot.img 是 FIT image 格式（关键认知）

Luckfox 250802 的 `boot.img` 不是普通 Rockchip 分区格式，而是 **FIT image（Flattened Image Tree）** 格式。

FIT image 结构：
- `offset 0x0`：FIT header DTB（1536 bytes），包含各组件的位置、大小、**SHA256 hash**
- `offset 0x800`：FDT blob（设备树主体，39020 bytes）
- `offset 0xa200`：kernel image
- `offset 0x321a00`：resource.img 中的 rk-kernel.dtb 副本

**直接替换 DTB 数据不生效的原因**：FIT header 存储了 DTB 的 SHA256 hash，U-Boot 验证不匹配时回退到旧 DTB。必须同时更新 FIT header 中的 hash 值。

**解决方案**：用 Python 脚本替换 DTB + 更新 FIT header hash（详见《GC2093完整配置指南.md》第六章）。

---

### 13.3 IQ 文件命名必须与 DTS 模组名对应

rkipc 会根据 DTS 中的属性**动态拼接** IQ 文件名：

```
{sensor}_{rockchip,camera-module-name}_{rockchip,camera-module-lens-name}.json
```

SDK 原始文件名（`gc2093_SIDA209300461_60IRC_F20.json`）通常与 DTS 配置不匹配，需要复制改名：

```bash
# 先确认 DTS 中的模组名，再创建对应文件
adb shell "cp /oem/usr/share/iqfiles/gc2093_SIDA209300461_60IRC_F20.json \
              /oem/usr/share/iqfiles/gc2093_LUCKFOX-GC2093_30IRC-F20.json"
```

---

### 13.4 rkipc 在 GC2093 下的 JPEG 编码器崩溃

**现象**：rkipc 启动数秒后自动退出，日志末尾有：
```
[video.c][rkipc_get_jpeg]:RK_MPI_VENC_GetStream timeout a004800e
```

**解决**：在 rkipc ini 文件中禁用 JPEG：
```ini
[video.source]
enable_jpeg = 0
```

---

### 13.5 RkLunch.sh 没有 gc2093 传感器检测逻辑

官方 `RkLunch.sh` 只检测 mia1321/mis5001/sc530ai/sc4336/sc3336 等默认传感器，不检测 gc2093，导致 rkipc 找不到对应 ini 文件而不启动。需手动添加：

```bash
# 在 sc3336 检测块之前插入
lsmod | grep gc2093
if [ $? -eq 0 ]; then
    ln -s -f /oem/usr/share/rkipc-gc2093-200w.ini $default_rkipc_ini
fi
```

---

### 13.6 macOS 上 rkdeveloptool db 在 maskrom 模式挂起

**现象**：`rkdeveloptool db download.bin` 无任何输出，一直挂起（loader 模式正常）。

**原因**：macOS USB bulk transfer 在 maskrom 模式（DDR 未初始化）下的兼容性问题。

**解决**：优先通过 `adb reboot loader` 进入 loader 模式操作；必须从 maskrom 恢复时多次重试通常可成功。

---

### 13.7 RTSP 无法连接（macOS + VPN 环境）

**现象**：RTSP 端口 554 在 board 上正常监听，但 Mac 侧无法连接 `172.32.0.93:554`。

**原因**：macOS VPN（Clash/Surge 等）将 172.32.x.x 段路由到 VPN 隧道而非 USB RNDIS 接口。

**解决**：用 ADB 端口转发绕过路由：
```bash
adb forward tcp:15554 tcp:554
ffplay rtsp://localhost:15554/live/0
```

---

*详细步骤和完整脚本参考：[GC2093完整配置指南.md](./GC2093完整配置指南.md)*

---

## Q14. SDK 源码不完整，无法编译完整 kernel

**现象**：`make` 时报错类似：
```
make[2]: *** No rule to make target 'net/netfilter/xt_DSCP.o', needed by ...
```

**原因**：Luckfox Pico SDK 是混合形式，部分内核子系统（USB、MMC、netfilter 等）仅提供预编译二进制，对应源文件缺失（经 2026-07 实测约 166 个源文件缺失）。

**影响**：
- ✅ 可以编译 DTB（DTS 源文件完整）
- ✅ 可以编译摄像头驱动模块（gc2093.ko 等）
- ❌ 无法编译完整 kernel

**绕过已知缺失模块**（针对 defconfig 报错）：
```bash
cd sysdrv/source/kernel
./scripts/config --disable CONFIG_NETFILTER_XT_TARGET_DSCP
./scripts/config --disable CONFIG_NETFILTER_XT_MATCH_DSCP
./scripts/config --disable CONFIG_IP6_NF_MATCH_HL
./scripts/config --disable CONFIG_IP6_NF_TARGET_HL
./scripts/config --disable CONFIG_IP_NF_TARGET_TTL
./scripts/config --disable CONFIG_IP_NF_MATCH_TTL
```

---

## Q15. macOS HFS+ 大小写不敏感导致内核编译失败

**现象**：Docker 容器内（`--platform linux/amd64`，SDK 目录挂载自 macOS）编译报错：
```
No rule to make target 'net/netfilter/xt_HL.o'
```

**原因**：macOS 默认文件系统（HFS+/APFS）**大小写不敏感**。SDK 中存在 `xt_hl.c`（小写），但 Kconfig 的 `select CONFIG_NETFILTER_XT_TARGET_HL` 对应的 Makefile 规则需要 `xt_HL.c`（大写）。在 macOS 挂载点上，Linux 内核构建系统找不到大写版本。

**解决**：在 defconfig 中禁用触发 `xt_HL` 的所有上层配置（见 Q14），或将整个 SDK 目录复制到 Linux 原生文件系统上再编译。

> 此问题**只影响完整 kernel 编译**。单独编译 DTB 或 gc2093.ko 不受影响。

---

## Q16. SDK 编译的 DTB 比原始 DTB 大，无法直接打入 boot.img

**现象**：SDK 修改 DTS 后编译的 DTB（42729 bytes）大于官方 boot.img 中的原始 DTB（39020 bytes），第六节的 Python patch 脚本会因 `new_dtb > ORIG_SIZE` 而产生错误或截断。

**原因**：SDK 的 DTS 文件比官方镜像内置的 DTB 包含更多节点（完整 SoC 描述），编译产物自然更大。

**解决方案**：用 `mkimage`（u-boot-tools）从零重建 FIT image，不受原始大小约束：

```bash
# Docker 环境中执行
apt-get install -y u-boot-tools

# 1. 从官方 boot.img 提取 kernel 和 resource
python3 - << 'EOF'
import struct, sys

boot = open('/official/boot.img', 'rb').read()
# 验证 FIT magic
assert struct.unpack_from('>I', boot, 0)[0] == 0xd00dfeed

KERNEL_OFF  = 0xa200
RESOURCE_OFF = 0x321200

open('/tmp/kernel.img',   'wb').write(boot[KERNEL_OFF:RESOURCE_OFF])
open('/tmp/resource.img', 'wb').write(boot[RESOURCE_OFF:])
print(f"kernel: {RESOURCE_OFF - KERNEL_OFF} bytes")
print(f"resource: {len(boot) - RESOURCE_OFF} bytes")
EOF

# 2. 创建 FIT image 描述文件（boot.its）
cat > /tmp/boot.its << 'ITS'
/dts-v1/;
/ {
    description = "RV1106 custom FIT";
    #address-cells = <1>;
    images {
        kernel {
            description = "Linux kernel";
            data = /incbin/("/tmp/kernel.img");
            type = "kernel";
            arch = "arm";
            os = "linux";
            compression = "none";
            load = <0x00408000>;
            entry = <0x00408000>;
            hash { algo = "sha256"; };
        };
        fdt {
            description = "GC2093 DTB";
            data = /incbin/("/tmp/new.dtb");
            type = "flat_dt";
            arch = "arm";
            compression = "none";
            hash { algo = "sha256"; };
        };
        resource {
            description = "resource image";
            data = /incbin/("/tmp/resource.img");
            type = "multi";
            arch = "arm";
            os = "linux";
            compression = "none";
            load = <0>;
            entry = <0>;
            hash { algo = "sha256"; };
        };
    };
    configurations {
        default = "config";
        config {
            description = "RV1106 config";
            kernel = "kernel";
            fdt = "fdt";
        };
    };
};
ITS

# 3. 打包
cp /path/to/rv1106g-luckfox-pico-zero-gc2093.dtb /tmp/new.dtb
mkimage -f /tmp/boot.its /tmp/boot-custom.img
ls -lh /tmp/boot-custom.img
```

> ⚠️ `load`/`entry` 地址 `0x00408000` 需与官方镜像一致，可通过 `dumpimage -l boot.img` 确认。

---

*详细步骤参考：[SDK编译指南.md](./SDK编译指南.md)，[GC2093完整配置指南.md](./GC2093完整配置指南.md)*
