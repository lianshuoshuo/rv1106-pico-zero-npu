/**
 * gc2093_test.cpp - GC2093 摄像头快速验证程序
 *
 * 用途：板端独立运行，验证 GC2093 能否正常采图
 * 编译：arm-rockchip830-linux-uclibcgnueabihf-g++ gc2093_test.cpp -o gc2093_test \
 *        -I${SDK}/sysdrv/source/mpp/include \
 *        -L${SDK}/sysdrv/source/mpp/lib \
 *        -lrockit -lrkaiq -pthread
 * 板端运行：
 *        RkLunch-stop.sh
 *        ./gc2093_test
 * 预期输出：5秒内持续打印 "frame #N pts=xxxx"，帧率约 30fps
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

// RKMPI 头文件（板端 SDK 提供）
#include "rk_mpi_sys.h"
#include "rk_mpi_vi.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_mb.h"
#include "rk_comm_video.h"

// ─── ISP 函数前向声明（避免 rkaiq 完整依赖）───
typedef enum {
    RK_AIQ_WORKING_MODE_NORMAL = 0,
} rk_aiq_working_mode_t;

#ifdef __cplusplus
extern "C" {
#endif
RK_S32 SAMPLE_COMM_ISP_Init(RK_S32 CamId, rk_aiq_working_mode_t WDRMode,
                             RK_BOOL MultiSensor, const char *iq_file_dir);
RK_S32 SAMPLE_COMM_ISP_Run(RK_S32 CamId);
RK_S32 SAMPLE_COMM_ISP_Stop(RK_S32 CamId);
#ifdef __cplusplus
}
#endif

// ─── 常量 ───────────────────────────────
#define CAM_ID       0
#define VI_DEV_ID    0
#define VI_PIPE_ID   0
#define VI_CHN_ID    0
#define VENC_CHN_ID  0
#define IQ_PATH      "/etc/iqfiles"

#define TEST_WIDTH   1920
#define TEST_HEIGHT  1080
#define TEST_FPS     30
#define TEST_BITRATE 4096   // kbps
#define TEST_SECS    5      // 验证时长

// ─── 全局 ───────────────────────────────
static volatile int g_stop = 0;

static void sig_handler(int sig) {
    (void)sig;
    g_stop = 1;
}

// ─── 工具函数 ─────────────────────────────
static uint64_t now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// ─── ISP 初始化 ───────────────────────────
static int test_isp_init(void) {
    RK_S32 ret = SAMPLE_COMM_ISP_Init(
        CAM_ID, RK_AIQ_WORKING_MODE_NORMAL, RK_FALSE, IQ_PATH);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[TEST] ISP Init failed: 0x%x\n"
                "       请确认 /etc/iqfiles/ 下有 gc2093_*.xml\n", ret);
        return -1;
    }
    ret = SAMPLE_COMM_ISP_Run(CAM_ID);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[TEST] ISP Run failed: 0x%x\n", ret);
        SAMPLE_COMM_ISP_Stop(CAM_ID);
        return -1;
    }
    printf("[TEST] ✓ ISP 就绪\n");
    return 0;
}

// ─── VI 初始化 ────────────────────────────
static int test_vi_init(void) {
    RK_S32 ret;

    ret = RK_MPI_VI_EnableDev(VI_DEV_ID);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[TEST] VI EnableDev failed: 0x%x\n", ret);
        return -1;
    }

    VI_DEV_BIND_PIPE_S bp;
    memset(&bp, 0, sizeof(bp));
    bp.u32Num = 1;
    bp.PipeId[0] = VI_PIPE_ID;
    RK_MPI_VI_SetDevBindPipe(VI_DEV_ID, &bp);

    VI_CHN_ATTR_S attr;
    memset(&attr, 0, sizeof(attr));
    attr.stSize.u32Width            = TEST_WIDTH;
    attr.stSize.u32Height           = TEST_HEIGHT;
    attr.enPixelFormat              = RK_FMT_YUV420SP;
    attr.u32Depth                   = 2;
    attr.enCompressMode             = COMPRESS_MODE_NONE;
    attr.stFrameRate.s32SrcFrameRate = TEST_FPS;
    attr.stFrameRate.s32DstFrameRate = TEST_FPS;

    ret = RK_MPI_VI_SetChnAttr(VI_PIPE_ID, VI_CHN_ID, &attr);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[TEST] VI SetChnAttr failed: 0x%x\n", ret);
        RK_MPI_VI_DisableDev(VI_DEV_ID);
        return -1;
    }

    ret = RK_MPI_VI_EnableChn(VI_PIPE_ID, VI_CHN_ID);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[TEST] VI EnableChn failed: 0x%x\n", ret);
        RK_MPI_VI_DisableDev(VI_DEV_ID);
        return -1;
    }

    printf("[TEST] ✓ VI 通道就绪 (%dx%d@%dfps)\n", TEST_WIDTH, TEST_HEIGHT, TEST_FPS);
    return 0;
}

// ─── VENC 初始化（H.264 CBR）────────────────
static int test_venc_init(void) {
    VENC_CHN_ATTR_S attr;
    memset(&attr, 0, sizeof(attr));

    attr.stVencAttr.enType       = RK_VIDEO_ID_AVC;
    attr.stVencAttr.u32Profile   = H264E_PROFILE_MAIN;
    attr.stVencAttr.u32PicWidth  = TEST_WIDTH;
    attr.stVencAttr.u32PicHeight = TEST_HEIGHT;
    attr.stVencAttr.u32VirWidth  = TEST_WIDTH;
    attr.stVencAttr.u32VirHeight = TEST_HEIGHT;
    attr.stVencAttr.u32BufSize   = TEST_WIDTH * TEST_HEIGHT * 3 / 2;

    attr.stRcAttr.enRcMode                    = VENC_RC_MODE_H264CBR;
    attr.stRcAttr.stH264Cbr.u32BitRate        = TEST_BITRATE;
    attr.stRcAttr.stH264Cbr.u32Gop           = TEST_FPS * 2;
    attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum   = TEST_FPS;
    attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum  = TEST_FPS;
    attr.stRcAttr.stH264Cbr.u32StatTime       = 1;

    RK_S32 ret = RK_MPI_VENC_CreateChn(VENC_CHN_ID, &attr);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[TEST] VENC CreateChn failed: 0x%x\n", ret);
        return -1;
    }

    VENC_RECV_PIC_PARAM_S recvParam;
    memset(&recvParam, 0, sizeof(recvParam));
    recvParam.s32RecvPicNum = -1;

    ret = RK_MPI_VENC_StartRecvFrame(VENC_CHN_ID, &recvParam);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[TEST] VENC StartRecvFrame failed: 0x%x\n", ret);
        RK_MPI_VENC_DestroyChn(VENC_CHN_ID);
        return -1;
    }

    printf("[TEST] ✓ VENC H.264 %dkbps 就绪\n", TEST_BITRATE);
    return 0;
}

// ─── VI→VENC 绑定 ─────────────────────────
static int test_bind(void) {
    MPP_CHN_S src, dst;
    src.enModId  = RK_ID_VI;   src.s32DevId = VI_DEV_ID;   src.s32ChnId = VI_CHN_ID;
    dst.enModId  = RK_ID_VENC; dst.s32DevId = 0;           dst.s32ChnId = VENC_CHN_ID;

    RK_S32 ret = RK_MPI_SYS_Bind(&src, &dst);
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[TEST] SYS_Bind VI->VENC failed: 0x%x\n", ret);
        return -1;
    }
    printf("[TEST] ✓ VI->VENC 绑定完成\n");
    return 0;
}

// ─── 资源释放 ─────────────────────────────
static void test_cleanup(void) {
    MPP_CHN_S src, dst;
    src.enModId  = RK_ID_VI;   src.s32DevId = VI_DEV_ID;   src.s32ChnId = VI_CHN_ID;
    dst.enModId  = RK_ID_VENC; dst.s32DevId = 0;           dst.s32ChnId = VENC_CHN_ID;
    RK_MPI_SYS_UnBind(&src, &dst);

    RK_MPI_VENC_StopRecvFrame(VENC_CHN_ID);
    RK_MPI_VENC_DestroyChn(VENC_CHN_ID);
    RK_MPI_VI_DisableChn(VI_PIPE_ID, VI_CHN_ID);
    RK_MPI_VI_DisableDev(VI_DEV_ID);
    SAMPLE_COMM_ISP_Stop(CAM_ID);
    RK_MPI_SYS_Exit();
    printf("[TEST] 资源已释放\n");
}

// ─── main ────────────────────────────────
int main(void) {
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    printf("=== GC2093 点亮验证 %dx%d@%dfps ===\n",
           TEST_WIDTH, TEST_HEIGHT, TEST_FPS);

    // 初始化 MPI 系统
    RK_S32 ret = RK_MPI_SYS_Init();
    if (ret != RK_SUCCESS) {
        fprintf(stderr, "[TEST] RK_MPI_SYS_Init failed: 0x%x\n", ret);
        return 1;
    }

    if (test_isp_init() != 0)  { RK_MPI_SYS_Exit(); return 1; }
    if (test_vi_init()  != 0)  { SAMPLE_COMM_ISP_Stop(CAM_ID); RK_MPI_SYS_Exit(); return 1; }
    if (test_venc_init() != 0) { test_cleanup(); return 1; }
    if (test_bind()      != 0) { test_cleanup(); return 1; }

    printf("[TEST] GC2093 已点亮，采集 %d 秒...\n", TEST_SECS);

    uint64_t t_start = now_ms();
    uint64_t frame_no = 0;
    uint64_t total_bytes = 0;

    while (!g_stop && (now_ms() - t_start) < (uint64_t)TEST_SECS * 1000) {
        VENC_STREAM_S stream;
        memset(&stream, 0, sizeof(stream));

        ret = RK_MPI_VENC_GetStream(VENC_CHN_ID, &stream, 500);
        if (ret != RK_SUCCESS) continue;

        for (RK_U32 i = 0; i < stream.u32PackCount; i++) {
            total_bytes += stream.pstPack[i].u32Len;
        }
        RK_U64 pts = stream.pstPack[0].u64PTS;
        RK_MPI_VENC_ReleaseStream(VENC_CHN_ID, &stream);

        frame_no++;
        if (frame_no % TEST_FPS == 0) {  // 每秒打印一次统计
            uint64_t elapsed = now_ms() - t_start;
            printf("[TEST] frame #%llu  pts=%llu  elapsed=%llums  %.1f fps  %.1f KB/s\n",
                   (unsigned long long)frame_no,
                   (unsigned long long)pts,
                   (unsigned long long)elapsed,
                   frame_no * 1000.0 / elapsed,
                   total_bytes / 1024.0 / (elapsed / 1000.0));
        }
    }

    printf("[TEST] 结束：共 %llu 帧，%.1f fps\n",
           (unsigned long long)frame_no,
           frame_no * 1000.0 / (now_ms() - t_start));

    test_cleanup();
    return (frame_no > 0) ? 0 : 2;  // 无帧=退出码2，方便脚本检测
}
