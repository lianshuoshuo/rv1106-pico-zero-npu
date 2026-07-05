# SDK 编译指南

> 更新日期：2026-07-04  
> 环境要求：Ubuntu 22.04 x86_64（原生，不支持虚拟机/WSL2 直接编译）

---

## 一、获取 SDK

```bash
# 国内推荐（Gitee）
git clone https://gitee.com/LuckfoxTECH/luckfox-pico.git

# 或 GitHub
git clone https://github.com/LuckfoxTECH/luckfox-pico.git
```

---

## 二、顶层目录结构

| 目录/文件 | 说明 |
|----------|------|
| `build.sh` | SDK 主编译脚本 |
| `media/` | 多媒体编解码、ISP 算法（可独立编译） |
| `sysdrv/` | U-Boot、kernel、rootfs（可独立编译） |
| `project/` | 参考应用、编译配置及脚本 |
| `output/` | 编译产物存放目录 |
| `tools/` | 镜像打包与烧录工具 |

---

## 三、编译产物（output/）

```
output/
├── image/
│   ├── download.bin     # 仅下载到内存的烧录通信程序
│   ├── env.img          # 分区表与启动参数
│   ├── uboot.img
│   ├── idblock.img      # loader 镜像
│   ├── boot.img         # kernel 镜像
│   ├── rootfs.img
│   └── userdata.img
└── out/
    ├── app_out/         # 参考应用编译结果
    ├── media_out/
    ├── rootfs_xxx/      # 文件系统打包目录
    ├── sysdrv_out/
    └── userdata/
```

---

## 四、安装编译依赖

```bash
sudo apt update
sudo apt-get install -y git ssh make gcc gcc-multilib g++-multilib \
  module-assistant expect g++ gawk texinfo libssl-dev bison flex \
  fakeroot cmake unzip gperf autoconf device-tree-compiler \
  libncurses5-dev pkg-config bc python-is-python3 passwd openssl \
  openssh-server openssh-client vim file cpio rsync curl
```

---

## 五、编译流程

### 全量编译

```bash
./build.sh lunch   # 交互式选择：板型 / 启动介质 / 系统版本
./build.sh         # 全量编译
```

硬件型号选项包括：RV1106 Pico Zero、Pro、Max、Ultra、Pi、86Panel 等。

### 单独编译组件

```bash
# 内核
./build.sh clean kernel && ./build.sh kernel

# U-Boot
./build.sh clean uboot && ./build.sh uboot

# 根文件系统
./build.sh clean rootfs && ./build.sh rootfs
```

### 重新打包固件（不重新编译）

```bash
./build.sh firmware
```

---

## 六、BoardConfig 关键配置项

位于 `project/cfg/BoardConfig_IPC/` 目录：

| 变量 | 含义 |
|------|------|
| `RK_BOOTARGS_CMA_SIZE` | 摄像头内存分配（不用摄像头可设为 1M） |
| `RK_KERNEL_DTS` | 设备树文件路径 |
| `RK_BOOT_MEDIUM` | 启动介质：`sd_card` / `spi_nand` / `eMMC` |
| `RK_PARTITION_CMD_IN_ENV` | 分区表配置 |
| `LF_TARGET_ROOTFS` | 目标根文件系统类型 |
| `RK_BUILDROOT_DEFCONFIG` | Buildroot 配置文件 |
| `RK_POST_OVERLAY` | 打包时叠加的 overlay 目录 |

---

## 七、Overlay 机制（自定义文件打包）

将自定义文件打入根文件系统的方式：

1. 在 `project/cfg/BoardConfig_IPC/overlay/` 下新建目录（如 `custom-overlay`）
2. 按目标文件系统路径放置文件（如 `etc/ssh/sshd_config`）
3. 在 `BoardConfig.mk` 中配置：
   ```bash
   export RK_POST_OVERLAY="custom-overlay"
   ```
4. 执行 `./build.sh firmware` 生成含 overlay 的镜像

---

## 八、交叉编译

### 工具链选择

| 目标系统 | 工具链 |
|---------|--------|
| Buildroot | `arm-rockchip830-linux-uclibcgnueabihf` |
| Ubuntu | `gcc-arm-11.2-2022.02-x86_64-arm-none-linux-gnueabihf` |

### 安装与配置（以 Buildroot 为例）

```bash
tar zxvf arm-rockchip830-linux-uclibcgnueabihf.tar.gz -C ~/
export PATH=~/arm-rockchip830-linux-uclibcgnueabihf/bin:$PATH
```

### 编译示例

```bash
# 直接编译
arm-rockchip830-linux-uclibcgnueabihf-gcc hello.c -o hello

# Makefile 示例
CC := ~/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc
hello: hello.c
	$(CC) $^ -o $@
```

> Makefile 缩进必须使用 Tab 键，不能用空格。

### 传输到开发板

```bash
scp hello root@<板子IP>:/root          # Buildroot
scp hello pico@<板子IP>:/home/pico     # Ubuntu
```

---

## 九、常见问题

**WSL2 报 PATH 含空格/制表符错误：**
```bash
export PATH=$(echo "$PATH" | tr -d ' \t\n')
```

**Buildroot 软件包下载慢/失败：**
下载官方离线包后校验并解压：
```bash
sha256sum -c dl.tar.bz2.sha256
tar -xjvf dl.tar.bz2 -C luckfox-pico/sysdrv/source/buildroot/buildroot-2023.02.6/
```

> 编译过程中避免使用 `sudo`，否则文件权限变更会导致编译失败。
