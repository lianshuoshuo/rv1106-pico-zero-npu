#include "video_stream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// RKMPI 头文件（板端 Luckfox SDK 提供）
#include "rk_mpi_sys.h"
#include "rk_mpi_vi.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_mb.h"
#include "rk_comm_video.h"
// ISP/AEC/AWB 算法控制
#include "rkaiq/uAPI2/rk_aiq_user_api2_sysctl.h"

// ========== 硬件通道常量 ==========
#define CAM_ID      0        // GC2093 摄像头 ID
#define VI_DEV_ID   0        // VI 设备号
#define VI_PIPE_ID  0        // VI pipe 号
#define VI_CHN_ID   0        // VI 通道号
#define VENC_CHN_ID 0        // VENC 通道号（H.264）

// IQ 标定文件目录（板端路径）
#define IQ_FILE_PATH "/etc/iqfiles"

// ========== 全局状态 ==========
static VideoConfig g_config;
static pthread_t g_stream_thread;
static volatile int g_stream_running = 0;
static uint64_t g_frame_count = 0;

// VENC 码流回调（由上层设置，用于 RTSP 推流）
static VideoFrameCallback g_frame_cb = NULL;
static void *g_frame_cb_userdata = NULL;

// ========== ISP 初始化 ==========
static int isp_init(void) {
    printf("[VideoStream] 初始化 ISP (CamId=%d, iq=%s)...\n", CAM_ID, IQ_FILE_PATH);

    // GC2093 需要加载 IQ 标定文件（位于板端 /etc/iqfiles/gc2093_*.xml）
    RK_S32 ret = SAMPLE_COMM_ISP_Init(
        CAM_ID,
        RK_AIQ_WORKING_MODE_NORMAL,  // 普通线性模式（非 HDR）
        RK_FALSE,                     // 单 sensor
        IQ_FILE_PATH
    );
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[VideoStream] SAMPLE_COMM_ISP_Init failed: 0x%x\n", ret);
        return -1;
    }

    ret = SAMPLE_COMM_ISP_Run(CAM_ID);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[VideoStream] SAMPLE_COMM_ISP_Run failed: 0x%x\n", ret);
        SAMPLE_COMM_ISP_Stop(CAM_ID);
        return -1;
    }

    printf("[VideoStream] ✓ ISP 已启动\n");
    return 0;
}

// ========== VI 初始化（GC2093 → NV12 采集） ==========
static int vi_init(uint32_t width, uint32_t height, uint32_t fps) {
    RK_S32 ret;

    // 1. 启用 VI 设备
    ret = RK_MPI_VI_EnableDev(VI_DEV_ID);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[VideoStream] RK_MPI_VI_EnableDev failed: 0x%x\n", ret);
        return -1;
    }

    // 2. 绑定 Pipe
    VI_DEV_BIND_PIPE_S stBindPipe;
    memset(&stBindPipe, 0, sizeof(stBindPipe));
    stBindPipe.u32Num = 1;
    stBindPipe.PipeId[0] = VI_PIPE_ID;
    ret = RK_MPI_VI_SetDevBindPipe(VI_DEV_ID, &stBindPipe);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[VideoStream] RK_MPI_VI_SetDevBindPipe failed: 0x%x\n", ret);
        RK_MPI_VI_DisableDev(VI_DEV_ID);
        return -1;
    }

    // 3. 设置 VI 通道属性
    VI_CHN_ATTR_S stChnAttr;
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.stSize.u32Width  = width;
    stChnAttr.stSize.u32Height = height;
    stChnAttr.enPixelFormat    = RK_FMT_YUV420SP;  // NV12
    stChnAttr.u32BufCount      = 3;                 // 三帧缓冲
    stChnAttr.u32Depth         = 1;
    stChnAttr.enCompressMode   = COMPRESS_MODE_NONE;
    stChnAttr.stFrameRate.s32SrcFrameRate = (RK_S32)fps;
    stChnAttr.stFrameRate.s32DstFrameRate = (RK_S32)fps;

    ret = RK_MPI_VI_SetChnAttr(VI_PIPE_ID, VI_CHN_ID, &stChnAttr);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[VideoStream] RK_MPI_VI_SetChnAttr failed: 0x%x\n", ret);
        RK_MPI_VI_DisableDev(VI_DEV_ID);
        return -1;
    }

    // 4. 启用通道
    ret = RK_MPI_VI_EnableChn(VI_PIPE_ID, VI_CHN_ID);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[VideoStream] RK_MPI_VI_EnableChn failed: 0x%x\n", ret);
        RK_MPI_VI_DisableDev(VI_DEV_ID);
        return -1;
    }

    printf("[VideoStream] ✓ VI 通道已启用 (%dx%d@%dfps NV12)\n", width, height, fps);
    return 0;
}

// ========== VENC 初始化（H.264 CBR 编码） ==========
static int venc_init(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate_kbps) {
    VENC_CHN_ATTR_S stAttr;
    memset(&stAttr, 0, sizeof(stAttr));

    // H.264 Main Profile
    stAttr.stVencAttr.enType        = RK_VIDEO_ID_AVC;
    stAttr.stVencAttr.u32Profile    = H264E_PROFILE_MAIN;
    stAttr.stVencAttr.u32PicWidth   = width;
    stAttr.stVencAttr.u32PicHeight  = height;
    stAttr.stVencAttr.u32VirWidth   = width;
    stAttr.stVencAttr.u32VirHeight  = height;
    // 输出缓冲：至少 2 帧 H.264 数据量
    stAttr.stVencAttr.u32BufSize    = width * height * 3 / 2;

    // CBR 码率控制
    stAttr.stRcAttr.enRcMode                     = VENC_RC_MODE_H264CBR;
    stAttr.stRcAttr.stH264Cbr.u32BitRate         = bitrate_kbps;
    stAttr.stRcAttr.stH264Cbr.u32Gop            = fps * 2;  // 2s 一个 IDR
    stAttr.stRcAttr.stH264Cbr.u32SrcFrameRate    = fps;
    stAttr.stRcAttr.stH264Cbr.fr32DstFrameRate   = fps;
    stAttr.stRcAttr.stH264Cbr.u32StatTime        = 1;        // 1s 统计窗口

    RK_S32 ret = RK_MPI_VENC_CreateChn(VENC_CHN_ID, &stAttr);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[VideoStream] RK_MPI_VENC_CreateChn failed: 0x%x\n", ret);
        return -1;
    }

    // 持续接收编码帧
    VENC_RECV_PIC_PARAM_S stRecvParam;
    memset(&stRecvParam, 0, sizeof(stRecvParam));
    stRecvParam.s32RecvPicNum = -1;

    ret = RK_MPI_VENC_StartRecvFrame(VENC_CHN_ID, &stRecvParam);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[VideoStream] RK_MPI_VENC_StartRecvFrame failed: 0x%x\n", ret);
        RK_MPI_VENC_DestroyChn(VENC_CHN_ID);
        return -1;
    }

    printf("[VideoStream] ✓ VENC H.264 CBR %dkbps GOP=%d\n", bitrate_kbps, fps * 2);
    return 0;
}

// ========== VI → VENC 硬件绑定 ==========
static int bind_vi_to_venc(void) {
    MPP_CHN_S stSrc, stDst;

    stSrc.enModId  = RK_ID_VI;
    stSrc.s32DevId = VI_DEV_ID;
    stSrc.s32ChnId = VI_CHN_ID;

    stDst.enModId  = RK_ID_VENC;
    stDst.s32DevId = 0;
    stDst.s32ChnId = VENC_CHN_ID;

    RK_S32 ret = RK_MPI_SYS_Bind(&stSrc, &stDst);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[VideoStream] RK_MPI_SYS_Bind VI->VENC failed: 0x%x\n", ret);
        return -1;
    }

    printf("[VideoStream] ✓ VI->VENC 硬件绑定完成\n");
    return 0;
}

// ========== 码流读取线程（VENC → 回调） ==========
static void *stream_thread_func(void *arg) {
    (void)arg;
    printf("[VideoStream] 码流线程已启动\n");

    while (g_stream_running) {
        VENC_STREAM_S stStream;
        memset(&stStream, 0, sizeof(stStream));

        // 阻塞等待编码完成，超时 500ms
        RK_S32 ret = RK_MPI_VENC_GetStream(VENC_CHN_ID, &stStream, 500);
        if (ret != RK_SUCCESS) {
            // 超时属正常（用于响应 g_stream_running 变化）
            continue;
        }

        // 遍历所有 NALU 包
        for (RK_U32 i = 0; i < stStream.u32PackCount; i++) {
            void   *pData = RK_MPI_MB_Handle2VirAddr(stStream.pstPack[i].pMbBlk);
            RK_U32  uLen  = stStream.pstPack[i].u32Len;
            RK_U64  uPts  = stStream.pstPack[i].u64PTS;

            // 通过回调送给 RTSP 服务器（或其他消费方）
            if (g_frame_cb && pData && uLen > 0) {
                g_frame_cb((const uint8_t *)pData, uLen, uPts, g_frame_cb_userdata);
            }
        }

        RK_MPI_VENC_ReleaseStream(VENC_CHN_ID, &stStream);
        g_frame_count++;
    }

    printf("[VideoStream] 码流线程已停止 (frames=%llu)\n",
           (unsigned long long)g_frame_count);
    return NULL;
}

// ========== 公开 API ==========

void video_stream_set_callback(VideoFrameCallback cb, void *userdata) {
    g_frame_cb = cb;
    g_frame_cb_userdata = userdata;
}

int video_stream_init(const VideoConfig *config) {
    if (!config) return -1;

    memcpy(&g_config, config, sizeof(VideoConfig));
    printf("[VideoStream] 初始化 %dx%d@%dfps %dkbps\n",
           config->width, config->height, config->fps, config->bitrate);

    // Step 1: RKMPI 系统初始化
    RK_S32 ret = RK_MPI_SYS_Init();
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[VideoStream] RK_MPI_SYS_Init failed: 0x%x\n", ret);
        return -1;
    }

    // Step 2: ISP（GC2093 AEC/AWB/AF 算法）
    if (isp_init() != 0) goto err_sys;

    // Step 3: VI 采集通道（GC2093 → NV12）
    if (vi_init(config->width, config->height, config->fps) != 0) goto err_isp;

    // Step 4: VENC H.264 编码器
    if (venc_init(config->width, config->height, config->fps, config->bitrate) != 0) goto err_vi;

    // Step 5: 硬件绑定 VI → VENC（零拷贝路径）
    if (bind_vi_to_venc() != 0) goto err_venc;

    printf("[VideoStream] ✓ GC2093 初始化完成，准备采图\n");
    return 0;

err_venc:
    RK_MPI_VENC_StopRecvFrame(VENC_CHN_ID);
    RK_MPI_VENC_DestroyChn(VENC_CHN_ID);
err_vi:
    RK_MPI_VI_DisableChn(VI_PIPE_ID, VI_CHN_ID);
    RK_MPI_VI_DisableDev(VI_DEV_ID);
err_isp:
    SAMPLE_COMM_ISP_Stop(CAM_ID);
err_sys:
    RK_MPI_SYS_Exit();
    return -1;
}

int video_stream_start(void) {
    if (g_stream_running) return -1;

    g_stream_running = 1;
    if (pthread_create(&g_stream_thread, NULL, stream_thread_func, NULL) != 0) {
        g_stream_running = 0;
        return -1;
    }

    printf("[VideoStream] 码流线程已启动\n");
    return 0;
}

void video_stream_stop(void) {
    if (!g_stream_running) return;

    g_stream_running = 0;
    pthread_join(g_stream_thread, NULL);
}

void video_stream_deinit(void) {
    // 先解绑
    MPP_CHN_S stSrc, stDst;
    stSrc.enModId  = RK_ID_VI;   stSrc.s32DevId = VI_DEV_ID;   stSrc.s32ChnId = VI_CHN_ID;
    stDst.enModId  = RK_ID_VENC; stDst.s32DevId = 0;           stDst.s32ChnId = VENC_CHN_ID;
    RK_MPI_SYS_UnBind(&stSrc, &stDst);

    // 按文档规定顺序释放
    RK_MPI_VENC_StopRecvFrame(VENC_CHN_ID);
    RK_MPI_VENC_DestroyChn(VENC_CHN_ID);
    RK_MPI_VI_DisableChn(VI_PIPE_ID, VI_CHN_ID);
    RK_MPI_VI_DisableDev(VI_DEV_ID);
    SAMPLE_COMM_ISP_Stop(CAM_ID);
    RK_MPI_SYS_Exit();

    printf("[VideoStream] 已释放所有 RKMPI 资源\n");
}

uint64_t video_stream_get_frame_count(void) {
    return g_frame_count;
}
