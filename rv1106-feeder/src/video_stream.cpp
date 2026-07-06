#include "video_stream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// 注意：以下代码需要实际的 RKMPI 头文件和库
// 这里提供框架代码，板端编译时需要链接 RKMPI 库

/*
#include "rk_mpi_sys.h"
#include "rk_mpi_vi.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_vpss.h"
*/

static VideoConfig g_config;
static pthread_t g_stream_thread;
static volatile int g_stream_running = 0;
static uint64_t g_frame_count = 0;

// 推流线程函数
static void* stream_thread_func(void *arg) {
    printf("推流线程已启动\n");

    // TODO: 实际的 RKMPI 推流逻辑
    /*
    while (g_stream_running) {
        // 1. 从 VI Ch0 获取帧
        VIDEO_FRAME_INFO_S stFrame;
        RK_S32 ret = RK_MPI_VI_GetChnFrame(0, 0, &stFrame, -1);
        if (ret != RK_SUCCESS) {
            continue;
        }

        // 2. 送入 VENC 编码
        ret = RK_MPI_VENC_SendFrame(0, &stFrame, -1);

        // 3. 获取编码后的码流
        VENC_STREAM_S stStream;
        ret = RK_MPI_VENC_GetStream(0, &stStream, -1);
        if (ret == RK_SUCCESS) {
            // 4. 推送到 RTSP 服务器
            for (int i = 0; i < stStream.u32PackCount; i++) {
                rtsp_server_push_frame(
                    stStream.pstPack[i].pu8Addr,
                    stStream.pstPack[i].u32Len,
                    stFrame.stVFrame.u64PTS
                );
            }
            RK_MPI_VENC_ReleaseStream(0, &stStream);
        }

        // 5. 释放帧
        RK_MPI_VI_ReleaseChnFrame(0, 0, &stFrame);

        g_frame_count++;
    }
    */

    // 临时模拟代码
    while (g_stream_running) {
        g_frame_count++;
        usleep(1000000 / g_config.fps);  // 模拟帧率
    }

    printf("推流线程已停止\n");
    return NULL;
}

int video_stream_init(const VideoConfig *config) {
    if (!config) {
        return -1;
    }

    memcpy(&g_config, config, sizeof(VideoConfig));

    printf("视频配置: %dx%d@%dfps, %dkbps\n",
           config->width, config->height, config->fps, config->bitrate);

    // TODO: 初始化 RKMPI
    /*
    RK_MPI_SYS_Init();

    // 配置 VI
    VI_CHN_ATTR_S stChnAttr;
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.stSize.u32Width = config->width;
    stChnAttr.stSize.u32Height = config->height;
    stChnAttr.enPixelFormat = RK_FMT_YUV420SP;
    stChnAttr.u32BufCount = 2;
    RK_MPI_VI_SetChnAttr(0, 0, &stChnAttr);
    RK_MPI_VI_EnableChn(0, 0);

    // 配置 VENC
    VENC_CHN_ATTR_S stVencAttr;
    memset(&stVencAttr, 0, sizeof(stVencAttr));
    stVencAttr.stVencAttr.enType = RK_VIDEO_ID_AVC;
    stVencAttr.stVencAttr.u32Profile = H264E_PROFILE_MAIN;
    stVencAttr.stVencAttr.u32PicWidth = config->width;
    stVencAttr.stVencAttr.u32PicHeight = config->height;
    stVencAttr.stVencAttr.u32BufSize = 2 * 1024 * 1024;

    stVencAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    stVencAttr.stRcAttr.stH264Cbr.u32BitRate = config->bitrate;
    stVencAttr.stRcAttr.stH264Cbr.u32Gop = config->fps;
    stVencAttr.stRcAttr.stH264Cbr.u32SrcFrameRate = config->fps;
    stVencAttr.stRcAttr.stH264Cbr.fr32DstFrameRate = config->fps;

    RK_MPI_VENC_CreateChn(0, &stVencAttr);

    // 绑定 VI -> VENC
    MPP_CHN_S stSrcChn, stDestChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32ChnId = 0;
    stDestChn.enModId = RK_ID_VENC;
    stDestChn.s32ChnId = 0;
    RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);

    RK_MPI_VENC_StartRecvFrame(0, NULL);
    */

    return 0;
}

int video_stream_start() {
    if (g_stream_running) {
        return -1;
    }

    g_stream_running = 1;
    int ret = pthread_create(&g_stream_thread, NULL, stream_thread_func, NULL);
    if (ret != 0) {
        g_stream_running = 0;
        return -1;
    }

    return 0;
}

void video_stream_stop() {
    if (!g_stream_running) {
        return;
    }

    g_stream_running = 0;
    pthread_join(g_stream_thread, NULL);
}

void video_stream_deinit() {
    // TODO: 清理 RKMPI 资源
    /*
    RK_MPI_VENC_StopRecvFrame(0);
    RK_MPI_VENC_DestroyChn(0);
    RK_MPI_VI_DisableChn(0, 0);
    RK_MPI_SYS_Exit();
    */
}

uint64_t video_stream_get_frame_count() {
    return g_frame_count;
}
