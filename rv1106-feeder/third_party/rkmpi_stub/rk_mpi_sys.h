/* rk_mpi_sys.h - Stub for local IDE (macOS). DO NOT use for actual compilation. */
#pragma once
#include "rk_type.h"

typedef enum {
    RK_ID_VI   = 1,
    RK_ID_VPSS = 3,
    RK_ID_VENC = 4,
    RK_ID_VDEC = 5,
    RK_ID_VO   = 6,
    RK_ID_AIO  = 7,
    RK_ID_BUTT
} MOD_ID_E;

typedef struct {
    MOD_ID_E enModId;
    RK_S32   s32DevId;
    RK_S32   s32ChnId;
} MPP_CHN_S;

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 RK_MPI_SYS_Init(void);
RK_S32 RK_MPI_SYS_Exit(void);
RK_S32 RK_MPI_SYS_Bind(const MPP_CHN_S *pstSrcChn, const MPP_CHN_S *pstDestChn);
RK_S32 RK_MPI_SYS_UnBind(const MPP_CHN_S *pstSrcChn, const MPP_CHN_S *pstDestChn);

#ifdef __cplusplus
}
#endif
