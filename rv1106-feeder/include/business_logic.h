#ifndef BUSINESS_LOGIC_H
#define BUSINESS_LOGIC_H

#include <stdint.h>
#include "rknn_detector.h"
#include "food_detector.h"

// 进食状态
typedef struct {
    int is_eating;                // 是否在进食
    const char *pet_id;           // 当前进食的宠物ID
    uint64_t start_time;          // 进食开始时间
    int consecutive_frames;       // 连续检测到的帧数
    uint64_t near_duration;       // 靠近碗的持续时间（帧数）
} EatingStatus;

// 定时任务配置
typedef struct {
    int hour;
    int minute;
    int enabled;
} ScheduleItem;

// 初始化业务逻辑模块
int business_logic_init();

// 加载定时任务配置（从 /userdata/config/schedule.json）
int business_logic_load_schedule(const char *config_path);

// 处理检测结果（主循环调用）
void business_logic_process(const DetectResult *detect_result, const char *pet_id);

// 获取当前进食状态
EatingStatus business_logic_get_eating_status();

// 获取当前食物余量
FoodLevel business_logic_get_food_level();

// 手动触发喂食（来自APP）
int business_logic_trigger_feed_manual(const char *pet_id);

// 检查定时任务（主循环定期调用）
void business_logic_check_schedule();

// 释放资源
void business_logic_deinit();

#endif // BUSINESS_LOGIC_H
