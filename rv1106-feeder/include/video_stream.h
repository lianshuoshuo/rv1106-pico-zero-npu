#ifndef VIDEO_STREAM_H
#define VIDEO_STREAM_H

#include <stdint.h>

// 视频流配置
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t bitrate;  // kbps
} VideoConfig;

// H.264 码流回调：每个 NALU 包触发一次
// data: 码流指针  len: 长度  pts: 时间戳(us)  userdata: 透传指针
typedef void (*VideoFrameCallback)(const uint8_t *data, uint32_t len,
                                   uint64_t pts, void *userdata);

// 注册 H.264 码流回调（在 video_stream_init 之前调用）
void video_stream_set_callback(VideoFrameCallback cb, void *userdata);

// 初始化视频流（ISP + VI + VENC）
int video_stream_init(const VideoConfig *config);

// 启动码流线程
int video_stream_start(void);

// 停止码流线程
void video_stream_stop(void);

// 释放所有 RKMPI 资源（顺序：解绑→VENC→VI→ISP→SYS）
void video_stream_deinit(void);

// 获取当前帧计数（用于诊断）
uint64_t video_stream_get_frame_count(void);

#endif // VIDEO_STREAM_H
