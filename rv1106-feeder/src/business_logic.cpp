#include "business_logic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#include "mqtt_client.h"
#include "uart_comm.h"

// 进食状态
static EatingStatus g_eating_status = {0};
static pthread_mutex_t g_eating_mutex = PTHREAD_MUTEX_INITIALIZER;

// 定时任务
#define MAX_SCHEDULES 10
static ScheduleItem g_schedules[MAX_SCHEDULES];
static int g_schedule_count = 0;

// 食物余量
static atomic_int g_food_level = FOOD_LEVEL_UNKNOWN;

// 上次进食时间（用于长时间未进食告警）
static time_t g_last_eating_time = 0;

// 喂食冷却（防止短时间重复触发）
static time_t g_last_feed_time = 0;
static const int FEED_COOLDOWN_SEC = 300;  // 5分钟冷却

// 碗ROI重叠计算
static float calculate_overlap(const DetectBox *box, const BowlROI *roi, int img_width, int img_height) {
    // 将归一化坐标转换为像素坐标
    int box_x = (int)(box->x * img_width);
    int box_y = (int)(box->y * img_height);
    int box_w = (int)(box->w * img_width);
    int box_h = (int)(box->h * img_height);

    // 计算交集
    int x1 = box_x > roi->x ? box_x : roi->x;
    int y1 = box_y > roi->y ? box_y : roi->y;
    int x2 = (box_x + box_w) < (roi->x + roi->w) ? (box_x + box_w) : (roi->x + roi->w);
    int y2 = (box_y + box_h) < (roi->y + roi->h) ? (box_y + box_h) : (roi->y + roi->h);

    if (x2 <= x1 || y2 <= y1) {
        return 0.0f;
    }

    int intersection = (x2 - x1) * (y2 - y1);
    int box_area = box_w * box_h;

    return (float)intersection / box_area;
}

int business_logic_init() {
    printf("初始化业务逻辑模块\n");

    memset(&g_eating_status, 0, sizeof(g_eating_status));
    g_last_eating_time = time(NULL);

    return 0;
}

int business_logic_load_schedule(const char *config_path) {
    if (!config_path) {
        return -1;
    }

    printf("加载定时任务配置: %s\n", config_path);

    // TODO: 实际的 JSON 解析
    /*
    FILE *fp = fopen(config_path, "r");
    if (!fp) {
        return -1;
    }

    // 解析 schedule.json
    cJSON *root = cJSON_Parse(...);
    cJSON *schedules = cJSON_GetObjectItem(root, "schedules");
    g_schedule_count = cJSON_GetArraySize(schedules);

    for (int i = 0; i < g_schedule_count && i < MAX_SCHEDULES; i++) {
        cJSON *item = cJSON_GetArrayItem(schedules, i);
        g_schedules[i].hour = cJSON_GetObjectItem(item, "hour")->valueint;
        g_schedules[i].minute = cJSON_GetObjectItem(item, "minute")->valueint;
        g_schedules[i].enabled = cJSON_GetObjectItem(item, "enabled")->valueint;
    }

    cJSON_Delete(root);
    fclose(fp);
    */

    // 临时模拟数据：每天早8点和晚6点
    g_schedule_count = 2;
    g_schedules[0].hour = 8;
    g_schedules[0].minute = 0;
    g_schedules[0].enabled = 1;
    g_schedules[1].hour = 18;
    g_schedules[1].minute = 0;
    g_schedules[1].enabled = 1;

    printf("已加载 %d 个定时任务\n", g_schedule_count);
    return 0;
}

void business_logic_process(const DetectResult *detect_result, const char *pet_id) {
    if (!detect_result || detect_result->count == 0) {
        // 没有检测到宠物
        pthread_mutex_lock(&g_eating_mutex);
        if (g_eating_status.is_eating) {
            g_eating_status.near_duration = 0;
            g_eating_status.consecutive_frames = 0;

            // 离开超过10秒（250帧@25fps）判定进食结束
            // 这里简化处理
            g_eating_status.is_eating = 0;

            time_t duration = time(NULL) - g_eating_status.start_time;
            mqtt_report_eating_end(g_eating_status.pet_id, duration);

            g_eating_status.pet_id = NULL;
        }
        pthread_mutex_unlock(&g_eating_mutex);
        return;
    }

    // 取置信度最高的检测框
    const DetectBox *best_box = &detect_result->boxes[0];
    for (int i = 1; i < detect_result->count; i++) {
        if (detect_result->boxes[i].confidence > best_box->confidence) {
            best_box = &detect_result->boxes[i];
        }
    }

    // 获取碗ROI
    BowlROI bowl_roi = food_detector_get_roi();

    // 计算重叠率（假设输入分辨率1920x1080）
    float overlap = calculate_overlap(best_box, &bowl_roi, 1920, 1080);

    pthread_mutex_lock(&g_eating_mutex);

    if (overlap > 0.3f) {
        // 宠物靠近碗
        g_eating_status.consecutive_frames++;
        g_eating_status.near_duration++;

        // 三重判定：连续5帧 + 重叠>30% + 停留>3秒（75帧@25fps）
        if (!g_eating_status.is_eating &&
            g_eating_status.consecutive_frames >= 5 &&
            g_eating_status.near_duration > 75) {

            // 判定进食开始
            g_eating_status.is_eating = 1;
            g_eating_status.pet_id = pet_id;
            g_eating_status.start_time = time(NULL);

            mqtt_report_eating_start(pet_id, g_eating_status.start_time);
            g_last_eating_time = g_eating_status.start_time;

            // 检查是否触发行为喂食
            time_t now = time(NULL);
            if (now - g_last_feed_time > FEED_COOLDOWN_SEC) {
                // 触发喂食
                uart_send_feed_command(pet_id);
                mqtt_report_feeding(pet_id, TRIGGER_INTENT, now);
                g_last_feed_time = now;
            }
        }
    } else {
        // 宠物远离碗
        g_eating_status.consecutive_frames = 0;
        g_eating_status.near_duration = 0;
    }

    pthread_mutex_unlock(&g_eating_mutex);
}

EatingStatus business_logic_get_eating_status() {
    pthread_mutex_lock(&g_eating_mutex);
    EatingStatus status = g_eating_status;
    pthread_mutex_unlock(&g_eating_mutex);
    return status;
}

FoodLevel business_logic_get_food_level() {
    return (FoodLevel)atomic_load(&g_food_level);
}

int business_logic_trigger_feed_manual(const char *pet_id) {
    if (!pet_id) {
        return -1;
    }

    time_t now = time(NULL);

    // 发送UART指令
    int ret = uart_send_feed_command(pet_id);
    if (ret != 0) {
        return -1;
    }

    // 上报MQTT
    mqtt_report_feeding(pet_id, TRIGGER_APP, now);

    g_last_feed_time = now;

    printf("手动触发喂食: pet=%s\n", pet_id);
    return 0;
}

void business_logic_check_schedule() {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    for (int i = 0; i < g_schedule_count; i++) {
        if (!g_schedules[i].enabled) {
            continue;
        }

        // 检查是否到达定时点（允许1分钟误差）
        if (local_time->tm_hour == g_schedules[i].hour &&
            local_time->tm_min == g_schedules[i].minute &&
            (now - g_last_feed_time > 60)) {

            // 触发定时喂食（如果有多只宠物，可以轮流或广播）
            // 这里简化处理，默认给第一只宠物
            const char *default_pet = "default";

            uart_send_feed_command(default_pet);
            mqtt_report_feeding(default_pet, TRIGGER_SCHEDULE, now);
            g_last_feed_time = now;

            printf("定时喂食触发: %02d:%02d\n",
                   g_schedules[i].hour, g_schedules[i].minute);
        }
    }

    // 检查长时间未进食告警（12小时 = 43200秒）
    if (now - g_last_eating_time > 43200) {
        mqtt_report_no_eating_alarm((now - g_last_eating_time) / 3600);
        g_last_eating_time = now;  // 避免重复告警
    }
}

void business_logic_deinit() {
    printf("业务逻辑模块已清理\n");
}
