# Linux 基础与系统配置

> 更新日期：2026-07-04

---

## 一、文件系统类型

| 存储介质 | 可读写格式 | 只读格式 |
|---------|-----------|---------|
| eMMC | ext4 | squashfs |
| SPI NAND / SLC NAND | ubifs | squashfs |
| SPI NOR | jffs2 | squashfs |

- **EXT4**：通用格式，支持大文件（最高 16TB），性能稳定
- **UBIFS / JFFS2**：专为 Flash 设计，具备擦写均衡和掉电保护

---

## 二、常用命令速查

### 目录与文件

```bash
ls -a              # 显示所有文件（含隐藏）
ls -lh             # 详细列表，文件大小人性化显示
cd ..              # 返回上一层
cd -               # 切换到上次目录
pwd                # 显示当前绝对路径
touch file.txt     # 新建文件
mkdir -p a/b       # 递归创建目录
cp -r src/ dst/    # 递归复制目录
mv file /dest      # 移动或重命名
rm -r dir/         # 递归删除目录
df -Th             # 显示挂载分区类型、大小、使用率
```

### 权限管理

```bash
chmod a+rw file         # 所有用户增加读写权限
chmod u=rw,go= file     # 仅所有者读写，其他清空
chmod 755 file          # 所有者 rwx，其他 r-x
chmod 644 file          # 所有者 rw-，其他 r--
chmod -R u+r dir/       # 递归修改目录权限
```

**数值对照：**

| 数值 | 权限 | 说明 |
|-----|------|------|
| 7 | rwx | 读+写+执行 |
| 6 | rw- | 读+写 |
| 5 | r-x | 读+执行 |
| 4 | r-- | 只读 |
| 0 | --- | 无权限 |

---

## 三、分区结构（eMMC）

典型分区布局：

```
env(256K) | idblock(256K@256K) | uboot(512K) | boot(4M) | oem(30M) | userdata(10M) | rootfs(80M)
```

挂载说明：

| 挂载点 | 设备 | 用途 |
|--------|------|------|
| `/` | `ubi0:rootfs` 或 ext4 分区 | 根文件系统 |
| `/oem` | `/dev/ubi4_0` | RK 驱动与库 |
| `/userdata` | `/dev/ubi5_0` | rkipc 配置、MAC 地址 |
| `tmpfs` | 内存 | 临时文件，断电清空 |

> `idblock` 偏移与大小固定，**禁止修改**。

---

## 四、开机自启动配置

### 创建启动脚本

在 `/etc/init.d/` 下新建 `S??<name>` 格式脚本（`S` 开头，数字越小越先执行）：

```bash
vim /etc/init.d/S99myapp
```

脚本基本结构：

```sh
#!/bin/sh
case $1 in
    start)
        # 启动命令
        /root/myapp &
        ;;
    stop)
        killall myapp
        ;;
    *)
        exit 1
        ;;
esac
```

赋予执行权限：

```bash
chmod 775 /etc/init.d/S99myapp
```

> 系统启动时 `rcS` 调用所有 `S` 开头脚本的 `start` 分支；关机时 `rcK` 调用 `stop` 分支。

---

## 五、静态 IP 配置

### 方式一：网线直连（无路由器）

```sh
# /etc/init.d/S99static 中 start 分支写入：
ifconfig eth0 192.168.10.200 netmask 255.255.252.0
route add default gw 192.168.11.1
echo "nameserver 114.114.114.114" > /etc/resolv.conf
```

### 方式二：连接路由器/交换机（需等待 DHCP）

```sh
#!/bin/sh
case $1 in
    start)
        # 等待 DHCP 分配 IP（最多 50 秒）
        for i in $(seq 1 10); do
            IP=$(ifconfig eth0 | grep "inet addr" | awk '{print $2}' | cut -d: -f2)
            [ -n "$IP" ] && break
            sleep 5
        done
        # 覆盖为静态 IP
        ifconfig eth0 192.168.10.66 netmask 255.255.252.0
        route add default gw 192.168.11.1
        echo "nameserver 114.114.114.114" > /etc/resolv.conf
        ;;
    stop)
        ;;
esac
```

> 静态 IP 需与路由器网段一致，且不得与已分配 IP 冲突。  
> 查看当前网关：`route -n`
