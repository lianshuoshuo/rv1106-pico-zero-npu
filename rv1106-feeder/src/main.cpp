#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "video_stream.h"
#include "rtsp_server.h"
#include "rknn_detector.h"
#include "reid_module.h"
#include "food_detector.h"
#include "mqtt_client.h"
#include "uart_comm.h"
#include "business_logic.h"

// 全局运行标志
static volatile int g_running = 1;

// 信号处理
void signal_handler(int sig) {
    printf("收到信号 %d，准备退出...\n", sig);
    g_running = 0;
}

int main(int argc, char *argv[]) {
    int ret = 0;

    printf("========================================\n");
    printf("  RV1106 智能宠物喂食器 v1.0.0\n");
    printf("========================================\n");

    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 1. 初始化视频流
    printf("[1/8] 初始化视频流...\n");
    VideoConfig video_cfg = {
        .width = 1920,
        .height = 1080,
        .fps = 25,
        .bitrate = 3072  // 3Mbps
    };
    ret = video_stream_init(&video_cfg);
    if (ret != 0) {
        fprintf(stderr, "视频流初始化失败: %d\n", ret);
        goto cleanup;
    }

    // 2. 初始化 RTSP 服务器
    printf("[2/8] 初始化 RTSP 服务器...\n");
    RtspConfig rtsp_cfg = {
        .port = 554,
        .stream_path = "/live/0"
    };
    ret = rtsp_server_init(&rtsp_cfg);
    if (ret != 0) {
        fprintf(stderr, "RTSP 服务器初始化失败: %d\n", ret);
        goto cleanup;
    }

    // 3. 初始化 RKNN 检测器
    printf("[3/8] 初始化 YOLOv5n 检测器...\n");
    ret = rknn_detector_init("/userdata/models/yolov5n.rknn");
    if (ret != 0) {
        fprintf(stderr, "检测器初始化失败: %d\n", ret);
        goto cleanup;
    }

    // 4. 初始化 Re-ID 模块
    printf("[4/8] 初始化 Re-ID 模块...\n");
    ret = reid_load_pets("/userdata/config/pets.json");
    if (ret != 0) {
        fprintf(stderr, "Re-ID 模块初始化失败: %d\n", ret);
        // 非致命错误，可继续运行
    }

    // 5. 初始化余量检测
    printf("[5/8] 初始化余量检测模块...\n");
    ret = food_detector_init("/userdata/models/mobilenet.rknn");
    if (ret != 0) {
        fprintf(stderr, "余量检测初始化失败: %d\n", ret);
        // 非致命错误，可继续运行
    }
    food_detector_load_roi("/userdata/config/bowl_roi.json");

    // 6. 初始化 MQTT 客户端
    printf("[6/8] 初始化 MQTT 客户端...\n");
    MqttConfig mqtt_cfg = {
        .broker = "mqtt.example.com",  // 从配置文件读取
        .port = 1883,
        .client_id = "rv1106_feeder_001",
        .username = NULL,
        .password = NULL
    };
    ret = mqtt_init(&mqtt_cfg);
    if (ret != 0) {
        fprintf(stderr, "MQTT 客户端初始化失败: %d\n", ret);
        // 非致命错误，可继续运行
    }

    // 7. 初始化 UART 通信
    printf("[7/8] 初始化 UART 通信...\n");
    UartConfig uart_cfg = {
        .device = "/dev/ttyS1",
        .baudrate = 115200
    };
    ret = uart_init(&uart_cfg);
    if (ret != 0) {
        fprintf(stderr, "UART 初始化失败: %d\n", ret);
        // 非致命错误，可继续运行
    }

    // 8. 初始化业务逻辑
    printf("[8/8] 初始化业务逻辑模块...\n");
    ret = business_logic_init();
    if (ret != 0) {
        fprintf(stderr, "业务逻辑初始化失败: %d\n", ret);
        goto cleanup;
    }
    business_logic_load_schedule("/userdata/config/schedule.json");

    // 启动服务
    printf("\n启动服务...\n");
    rtsp_server_start();
    video_stream_start();
    mqtt_start();

    printf("\n✅ 系统启动成功！\n");
    printf("   RTSP 推流: rtsp://<设备IP>:554/live/0\n");
    printf("   按 Ctrl+C 退出\n\n");

    // 主循环
    uint64_t frame_count = 0;
    uint64_t last_schedule_check = 0;
    uint64_t last_food_check = 0;

    while (g_running) {
        // 获取当前帧号
        frame_count = video_stream_get_frame_count();

        // 每秒检查一次定时任务
        if (frame_count - last_schedule_check > 25) {
            business_logic_check_schedule();
            last_schedule_check = frame_count;
        }

        // 每30秒检查一次余量（30秒 * 25fps = 750帧）
        if (frame_count - last_food_check > 750) {
            FoodLevel level = business_logic_get_food_level();
            if (level == FOOD_LEVEL_LOW || level == FOOD_LEVEL_EMPTY) {
                mqtt_report_food_level(level);
            }
            last_food_check = frame_count;
        }

        // 休眠避免空转
        usleep(100000);  // 100ms
    }

cleanup:
    printf("\n正在清理资源...\n");

    video_stream_stop();
    rtsp_server_stop();
    mqtt_stop();

    video_stream_deinit();
    rtsp_server_deinit();
    rknn_detector_deinit();
    reid_deinit();
    food_detector_deinit();
    mqtt_deinit();
    uart_deinit();
    business_logic_deinit();

    printf("程序已退出。\n");
    return ret;
}
