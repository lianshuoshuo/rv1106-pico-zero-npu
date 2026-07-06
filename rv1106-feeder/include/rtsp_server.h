#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#include <stdint.h>

// RTSP 服务器配置
typedef struct {
    uint16_t port;
    const char *stream_path;  // 例如 "/live/0"
} RtspConfig;

// 初始化 RTSP 服务器
int rtsp_server_init(const RtspConfig *config);

// 启动 RTSP 服务
int rtsp_server_start();

// 推送 H264 数据包
int rtsp_server_push_frame(const uint8_t *data, uint32_t size, uint64_t timestamp);

// 停止服务
void rtsp_server_stop();

// 释放资源
void rtsp_server_deinit();

#endif // RTSP_SERVER_H
