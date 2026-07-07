/* rk_mpi_vi.h - Stub for local IDE (macOS). DO NOT use for actual compilation. */
#pragma once
#include "rk_type.h"
#include "rk_comm_video.h"

typedef struct {
    RK_U32 u32Num;
    RK_U32 PipeId[4];
} VI_DEV_BIND_PIPE_S;

typedef struct {
    SIZE_S             stSize;
    PIXEL_FORMAT_E     enPixelFormat;
    COMPRESS_MODE_E    enCompressMode;
    RK_U32             u32BufCount;
    RK_U32             u32Depth;
    FRAME_RATE_CTRL_S  stFrameRate;
} VI_CHN_ATTR_S;

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 RK_MPI_VI_EnableDev(RK_S32 ViDev);
RK_S32 RK_MPI_VI_DisableDev(RK_S32 ViDev);
RK_S32 RK_MPI_VI_SetDevBindPipe(RK_S32 ViDev, const VI_DEV_BIND_PIPE_S *pstDevBindPipe);
RK_S32 RK_MPI_VI_SetChnAttr(RK_S32 ViPipe, RK_S32 ViChn, const VI_CHN_ATTR_S *pstChnAttr);
RK_S32 RK_MPI_VI_EnableChn(RK_S32 ViPipe, RK_S32 ViChn);
RK_S32 RK_MPI_VI_DisableChn(RK_S32 ViPipe, RK_S32 ViChn);
RK_S32 RK_MPI_VI_GetChnFrame(RK_S32 ViPipe, RK_S32 ViChn, VIDEO_FRAME_INFO_S *pstFrameInfo, RK_S32 s32MilliSec);
RK_S32 RK_MPI_VI_ReleaseChnFrame(RK_S32 ViPipe, RK_S32 ViChn, const VIDEO_FRAME_INFO_S *pstFrameInfo);

#ifdef __cplusplus
}
#endif
