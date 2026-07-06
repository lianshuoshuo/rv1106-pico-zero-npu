#ifndef FOOD_DETECTOR_H
#define FOOD_DETECTOR_H

#include <stdint.h>

// 食物余量等级
typedef enum {
    FOOD_LEVEL_FULL = 0,
    FOOD_LEVEL_HALF = 1,
    FOOD_LEVEL_LOW = 2,
    FOOD_LEVEL_EMPTY = 3,
    FOOD_LEVEL_UNKNOWN = 4
} FoodLevel;

// 碗ROI配置
typedef struct {
    int x, y, w, h;  // 像素坐标
} BowlROI;

// 初始化余量检测模型
int food_detector_init(const char *model_path);

// 加载碗ROI配置
int food_detector_load_roi(const char *config_path);

// 检测食物余量（输入裁剪后的碗图像 128x128）
FoodLevel food_detector_check(const uint8_t *bowl_image);

// 获取当前碗ROI
BowlROI food_detector_get_roi();

// 释放资源
void food_detector_deinit();

// 余量等级转字符串
const char* food_level_to_string(FoodLevel level);

#endif // FOOD_DETECTOR_H
