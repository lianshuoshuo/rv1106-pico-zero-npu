/* rk_mpi_venc.h - Stub for local IDE (macOS). DO NOT use for actual compilation. */
#pragma once
#include "rk_type.h"
#include "rk_comm_video.h"

/* H.264 profile */
typedef enum {
    H264E_PROFILE_BASELINE = 0,
    H264E_PROFILE_MAIN,
    H264E_PROFILE_HIGH,
    H264E_PROFILE_BUTT
} H264E_PROFILE_E;

/* 编码类型 */
typedef enum {
    RK_VIDEO_ID_AVC  = 96,   /* H.264 */
    RK_VIDEO_ID_HEVC = 265,  /* H.265 */
    RK_VIDEO_ID_JPEG = 26,
    RK_VIDEO_ID_BUTT
} RK_CODEC_ID_E;

/* 码率控制模式 */
typedef enum {
    VENC_RC_MODE_H264CBR = 1,
    VENC_RC_MODE_H264VBR,
    VENC_RC_MODE_H264AVBR,
    VENC_RC_MODE_BUTT
} VENC_RC_MODE_E;

/* H.264 CBR 参数 */
typedef struct {
    RK_U32 u32Gop;
    RK_U32 u32StatTime;
    RK_U32 u32SrcFrameRate;
    RK_U32 fr32DstFrameRate;
    RK_U32 u32BitRate;
} VENC_H264_CBR_S;

typedef struct {
    VENC_RC_MODE_E  enRcMode;
    VENC_H264_CBR_S stH264Cbr;
} VENC_RC_ATTR_S;

typedef struct {
    RK_CODEC_ID_E enType;
    RK_U32 u32Profile;
    RK_U32 u32PicWidth;
    RK_U32 u32PicHeight;
    RK_U32 u32VirWidth;
    RK_U32 u32VirHeight;
    RK_U32 u32BufSize;
} VENC_ATTR_S;

typedef struct {
    VENC_ATTR_S    stVencAttr;
    VENC_RC_ATTR_S stRcAttr;
} VENC_CHN_ATTR_S;

/* 码流 pack */
typedef struct {
    MB_BLK  pMbBlk;
    RK_U32  u32Len;
    RK_U64  u64PTS;
    RK_U32  u32Type;
} VENC_PACK_S;

typedef struct {
    VENC_PACK_S *pstPack;
    RK_U32       u32PackCount;
    RK_U32       u32Seq;
} VENC_STREAM_S;

typedef struct {
    RK_S32 s32RecvPicNum;
} VENC_RECV_PIC_PARAM_S;

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 RK_MPI_VENC_CreateChn(RK_S32 VencChn, const VENC_CHN_ATTR_S *pstAttr);
RK_S32 RK_MPI_VENC_DestroyChn(RK_S32 VencChn);
RK_S32 RK_MPI_VENC_StartRecvFrame(RK_S32 VencChn, const VENC_RECV_PIC_PARAM_S *pstRecvParam);
RK_S32 RK_MPI_VENC_StopRecvFrame(RK_S32 VencChn);
RK_S32 RK_MPI_VENC_GetStream(RK_S32 VencChn, VENC_STREAM_S *pstStream, RK_S32 s32MilliSec);
RK_S32 RK_MPI_VENC_ReleaseStream(RK_S32 VencChn, VENC_STREAM_S *pstStream);
RK_S32 RK_MPI_VENC_SendFrame(RK_S32 VencChn, const VIDEO_FRAME_INFO_S *pstFrame, RK_S32 s32MilliSec);

#ifdef __cplusplus
}
#endif
