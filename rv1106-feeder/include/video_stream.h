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

// 初始化视频流（VI + VENC）
int video_stream_init(const VideoConfig *config);

// 启动推流线程
int video_stream_start();

// 停止推流
void video_stream_stop();

// 释放资源
void video_stream_deinit();

// 获取当前帧号（用于调试）
uint64_t video_stream_get_frame_count();

#endif // VIDEO_STREAM_H
