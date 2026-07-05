# RKMPI 多媒体处理指南

> 更新日期：2026-07-04  
> RKMPI：Rockchip Media Process Interface

---

## 一、系统架构

```
应用层 → RKMPI 层 → OS 适配层 → Linux 系统层 → 硬件层
```

完整视频流水线：

```
ISP → VI 采集 → VPSS 格式转换 → [可选 RKNN 推理 / OpenCV 标注] → VENC H264 编码 → RTSP 推流
```

---

## 二、VI 模块（视频输入）

从 MIPI CSI 采集视频，内部结构：设备(dev) → 管道(pipe) → 通道(channel)

### 初始化

```c
SAMPLE_COMM_ISP_Init(CamId, hdr_mode, multi_sensor, iq_dir);  // ISP 算法初始化
SAMPLE_COMM_ISP_Run(CamId);                                    // 运行 ISP
RK_MPI_VI_EnableDev(devID);
RK_MPI_VI_SetDevBindPipe(devID, &stBindPipe);
RK_MPI_VI_SetChnAttr(PipeID, ChnID, &Chn_attr);
RK_MPI_VI_EnableChn(PipeID, ChnID);
```

### 绑定到下游模块

```c
MPP_CHN_S viChn;
viChn.enModId   = RK_ID_VI;
viChn.s32DevID  = DevID;
viChn.s32ChnID  = ChnID;
RK_MPI_SYS_Bind(&viChn, &otherChn);
```

### 关闭

```c
RK_MPI_VI_DisableChn(PipeID, ChnID);
RK_MPI_VI_DisableDev(DevID);
```

---

## 三、VPSS 模块（视频处理子系统）

支持缩放、旋转、镜像、像素格式转换、裁剪。组(group)控制输入，通道(channel)控制输出。

### 初始化

```c
RK_MPI_VPSS_CreateGrp(GrpID, &VpssGrpAttr);
RK_MPI_VPSS_SetChnAttr(GrpID, ChnID, &VpssChnAttr);
RK_MPI_VPSS_EnableChn(GrpID, ChnID);
RK_MPI_VPSS_StartGrp(GrpID);
```

### 获取帧数据

```c
RK_MPI_VPSS_GetChnFrame(GrpID, ChnID, &VpssFrame, -1);
void *data = RK_MPI_MB_Handle2VirAddr(VpssFrame.stVFrame.pMbBlk);
// 处理完后释放
RK_MPI_VPSS_ReleaseChnFrame(GrpID, ChnID, &VpssFrame);
```

### 关闭

```c
RK_MPI_VPSS_StopGrp(GrpID);
RK_MPI_VPSS_DestroyGrp(GrpID);
```

### 像素格式配置建议

| 链路 | 格式 |
|------|------|
| VI → VPSS 组输入 | `RK_FMT_YUV420SP` |
| VPSS 输出（送 RKNN） | `RK_FMT_RGB888` |

---

## 四、VENC 模块（视频编码）

支持 H.264、H.265、JPEG、MJPEG 多路独立编码。

### H.264 CBR 配置

```c
stAttr.stVencAttr.enType         = RK_VIDEO_ID_AVC;
stAttr.stVencAttr.u32Profile     = H264E_PROFILE_MAIN;
stAttr.stRcAttr.enRcMode         = VENC_RC_MODE_H264CBR;
stAttr.stRcAttr.stH264Cbr.u32BitRate = BitRate;
stAttr.stRcAttr.stH264Cbr.u32Gop    = Gop;  // GOP=1 时全为 I 帧

RK_MPI_VENC_CreateChn(ChnId, &stAttr);
RK_MPI_VENC_StartRecvFrame(ChnId, &stRecvParam);
```

### 获取编码帧

```c
RK_MPI_VENC_GetStream(ChnId, &stFrame, -1);
void *Data = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);
RK_U32 Len = stFrame.pstPack->u32Len;
// 处理完后释放
RK_MPI_VENC_ReleaseStream(ChnId, &stFrame);
```

### 关闭

```c
RK_MPI_VENC_StopRecvFrame(ChnId);
RK_MPI_VENC_DestroyChn(ChnId);
```

> VLC 拉流缓存不能低于 500ms。

---

## 五、内存管理（MB Pool）

```c
// 创建缓冲池
MB_POOL_CONFIG_S PoolCfg;
PoolCfg.u64MBSize   = width * height * channel;
PoolCfg.u32MBCnt    = num;
PoolCfg.enAllocType = MB_ALLOC_TYPE_DMA;

MB_POOL src_Pool = RK_MPI_MB_CreatePool(&PoolCfg);
MB_BLK  src_Blk  = RK_MPI_MB_GetMB(src_Pool, width * height * 3, RK_TRUE);

// 释放
RK_MPI_MB_ReleaseMB(src_Blk);
RK_MPI_MB_DestroyPool(src_Pool);
```

---

## 六、RTSP 推流

```c
// 初始化
g_rtsplive    = create_rtsp_demo(554);
g_rtsp_session = rtsp_new_session(g_rtsplive, "/live/0");
rtsp_set_video(g_rtsp_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
rtsp_sync_video_ts(g_rtsp_session, rtsp_get_reltime(), rtsp_get_ntptime());

// 推流循环
rtsp_tx_video(g_rtsp_session, (uint8_t *)pData, PackLen, PTS);
rtsp_do_event(g_rtsplive);

// 销毁
rtsp_del_demo(g_rtsplive);
```

VLC 拉流地址：`rtsp://<设备IP>/live/0`

> 当前不支持 MJPEG 推流。

---

## 七、VI → VPSS 绑定示例

```c
MPP_CHN_S stSrcChn, stVpssChn;

stSrcChn.enModId   = RK_ID_VI;
stSrcChn.s32ChnId  = 0;
stVpssChn.enModId  = RK_ID_VPSS;
stVpssChn.s32ChnId = 0;

RK_MPI_SYS_Bind(&stSrcChn, &stVpssChn);

// 发送帧到 VENC（阻塞发送保证完整性）
RK_MPI_VENC_SendFrame(vpssChn, &stVpssFrame, -1);
```

---

## 八、RKNN 推理集成方案对比

| 方案 | 帧率（人脸/目标） | 特点 |
|------|-----------------|------|
| opencv-mobile 绘制后送 VENC | ~11fps / ~7fps | 简单，直接在图像上绘制 |
| RGN/OSD 叠加到码流 | 更高 | 独立推理线程，内存占用较高 |

OSD 方案使用两个 VI 通道：通道 0 绑 VENC 推流，通道 1 绑 VPSS 做推理。

### 采集方式性能对比

| 方式 | 帧率 | 特点 |
|------|------|------|
| VI 组件采集 | ~23fps | 硬件加速，推荐 |
| opencv-mobile 采集 | ~9fps | 简单但性能损失明显 |

---

## 九、完整资源释放顺序

```c
RK_MPI_SYS_UnBind(...);
RK_MPI_VI_DisableChn(PipeID, ChnID);
RK_MPI_VI_DisableDev(DevID);
RK_MPI_VPSS_StopGrp(GrpID);
RK_MPI_VPSS_DestroyGrp(GrpID);
RK_MPI_VENC_StopRecvFrame(ChnId);
RK_MPI_VENC_DestroyChn(ChnId);
rtsp_del_demo(g_rtsplive);
SAMPLE_COMM_ISP_Stop(CamId);
RK_MPI_SYS_Exit();
release_rknn_model(&rknn_app_ctx);  // 若有 RKNN
```

---

## 十、编译与运行

```bash
git clone https://github.com/LuckfoxTECH/luckfox_pico_rkmpi_example.git
export LUCKFOX_SDK_PATH=<SDK 绝对路径>
./build.sh   # 选择目标 demo
```

板端运行前关闭默认后台程序：

```bash
RkLunch-stop.sh
chmod a+x <Demo>
./<Demo>
```
