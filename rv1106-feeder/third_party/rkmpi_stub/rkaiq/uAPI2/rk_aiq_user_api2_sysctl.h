/* rk_aiq_user_api2_sysctl.h - Stub for local IDE (macOS). DO NOT use for actual compilation. */
#pragma once
#include "../../rk_type.h"

typedef enum {
    RK_AIQ_WORKING_MODE_NORMAL = 0,
    RK_AIQ_WORKING_MODE_ISP_HDR2,
    RK_AIQ_WORKING_MODE_ISP_HDR3,
} rk_aiq_working_mode_t;

#ifdef __cplusplus
extern "C" {
#endif

/* 以下函数由 Luckfox SDK sample_comm_isp 封装 */
RK_S32 SAMPLE_COMM_ISP_Init(RK_S32 CamId,
                             rk_aiq_working_mode_t WDRMode,
                             RK_BOOL MultiSensor,
                             const char *iq_file_dir);

RK_S32 SAMPLE_COMM_ISP_Run(RK_S32 CamId);
RK_S32 SAMPLE_COMM_ISP_Stop(RK_S32 CamId);

#ifdef __cplusplus
}
#endif
