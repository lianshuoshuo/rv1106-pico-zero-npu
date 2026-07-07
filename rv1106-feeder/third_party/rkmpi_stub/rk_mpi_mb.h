/* rk_mpi_mb.h - Stub for local IDE (macOS). DO NOT use for actual compilation. */
#pragma once
#include "rk_type.h"

typedef enum {
    MB_ALLOC_TYPE_DMA = 0,
    MB_ALLOC_TYPE_BUTT
} MB_ALLOC_TYPE_E;

typedef struct {
    RK_U64          u64MBSize;
    RK_U32          u32MBCnt;
    MB_ALLOC_TYPE_E enAllocType;
} MB_POOL_CONFIG_S;

#ifdef __cplusplus
extern "C" {
#endif

void    *RK_MPI_MB_Handle2VirAddr(MB_BLK mb);
MB_POOL  RK_MPI_MB_CreatePool(const MB_POOL_CONFIG_S *pstPoolCfg);
RK_S32   RK_MPI_MB_DestroyPool(MB_POOL pool);
MB_BLK   RK_MPI_MB_GetMB(MB_POOL pool, RK_U64 u64Size, RK_U32 bBlock);
RK_S32   RK_MPI_MB_ReleaseMB(MB_BLK mb);

#ifdef __cplusplus
}
#endif
