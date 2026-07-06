#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stdint.h>
#include "food_detector.h"

// MQTT 配置
typedef struct {
    const char *broker;       // MQTT Broker 地址
    int port;                 // 端口（默认1883）
    const char *client_id;    // 客户端ID
    const char *username;     // 用户名（可选）
    const char *password;     // 密码（可选）
} MqttConfig;

// 喂食触发类型
typedef enum {
    TRIGGER_APP = 0,       // APP手动触发
    TRIGGER_SCHEDULE = 1,  // 定时任务
    TRIGGER_INTENT = 2     // 行为触发（进食意图）
} TriggerType;

// 初始化 MQTT 客户端
int mqtt_init(const MqttConfig *config);

// 启动 MQTT 连接
int mqtt_start();

// 上报喂食事件
int mqtt_report_feeding(const char *pet_id, TriggerType trigger, uint64_t timestamp);

// 上报进食开始
int mqtt_report_eating_start(const char *pet_id, uint64_t timestamp);

// 上报进食结束
int mqtt_report_eating_end(const char *pet_id, uint64_t duration_sec);

// 上报食物余量
int mqtt_report_food_level(FoodLevel level);

// 上报长时间未进食告警
int mqtt_report_no_eating_alarm(uint64_t hours);

// 停止 MQTT 客户端
void mqtt_stop();

// 释放资源
void mqtt_deinit();

#endif // MQTT_CLIENT_H
