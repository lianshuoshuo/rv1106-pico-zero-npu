/* rk_comm_video.h - Stub for local IDE (macOS). DO NOT use for actual compilation. */
#pragma once
#include "rk_type.h"

typedef enum {
    RK_FMT_YUV420SP = 0,   /* NV12 */
    RK_FMT_YUV420P,
    RK_FMT_RGB888,
    RK_FMT_BGR888,
    RK_FMT_ARGB8888,
    RK_FMT_BUTT
} PIXEL_FORMAT_E;

typedef enum {
    COMPRESS_MODE_NONE = 0,
    COMPRESS_AFBC_16x16,
    COMPRESS_MODE_BUTT
} COMPRESS_MODE_E;

typedef struct {
    RK_U32 u32Width;
    RK_U32 u32Height;
} SIZE_S;

typedef struct {
    RK_S32 s32SrcFrameRate;
    RK_S32 s32DstFrameRate;
} FRAME_RATE_CTRL_S;

typedef struct {
    MB_BLK   pMbBlk;
    RK_U32   u32Width;
    RK_U32   u32Height;
    PIXEL_FORMAT_E enPixelFormat;
    RK_U64   u64PTS;
    RK_U32   u32Stride[3];
} VIDEO_FRAME_S;

typedef struct {
    VIDEO_FRAME_S stVFrame;
    RK_U32  u32PoolId;
} VIDEO_FRAME_INFO_S;
