# NFS 挂载配置

> 更新日期：2026-07-04  
> 用途：将 Ubuntu 主机目录挂载到开发板，实现文件实时共享，便于开发调试

---

## 前置条件

- 开发板与主机处于同一网段，互相可以 ping 通
- 开发板已通过 SSH 或 ADB 登录

---

## 一、Ubuntu 主机端配置

### 1.1 安装 NFS 服务

```bash
sudo apt update
sudo apt install nfs-kernel-server
```

### 1.2 创建共享目录

```bash
sudo mkdir -p /home/nfs_share
sudo chmod 777 /home/nfs_share
```

### 1.3 配置导出规则

编辑 `/etc/exports`，添加：

```
/home/nfs_share *(rw,sync,no_subtree_check,no_root_squash)
```

参数说明：

| 参数 | 说明 |
|------|------|
| `rw` | 允许读写 |
| `sync` | 同步写入，保证数据一致性 |
| `no_subtree_check` | 禁用子树检查，提升性能 |
| `no_root_squash` | 允许客户端 root 用户保留 root 权限 |
| `*` | 允许所有 IP 访问（可改为指定 IP，如 `192.168.1.0/24`） |

### 1.4 生效配置

```bash
sudo exportfs -ra
sudo systemctl restart nfs-kernel-server
# 确认共享已生效
sudo exportfs -v
```

---

## 二、开发板端挂载

### 2.1 创建挂载点

```bash
mkdir -p /mnt/nfs
```

### 2.2 挂载远程目录

```bash
# 将 192.168.1.100 替换为实际主机 IP
mount -t nfs -o nolock 192.168.1.100:/home/nfs_share /mnt/nfs
```

### 2.3 验证挂载

```bash
df -h
mount | grep nfs
ls /mnt/nfs
```

---

## 三、开机自动挂载（可选）

在 `/etc/init.d/S99nfs` 中添加：

```sh
#!/bin/sh
case $1 in
    start)
        mount -t nfs -o nolock 192.168.1.100:/home/nfs_share /mnt/nfs
        ;;
    stop)
        umount /mnt/nfs
        ;;
esac
```

```bash
chmod +x /etc/init.d/S99nfs
```

---

## 四、常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 挂载超时 | 网络不通 | 检查 IP 地址，确认同网段 |
| Permission denied | exports 配置错误 | 检查 `/etc/exports`，重新 `exportfs -ra` |
| `nfs: server not responding` | NFS 服务未启动 | 主机执行 `sudo systemctl restart nfs-kernel-server` |
