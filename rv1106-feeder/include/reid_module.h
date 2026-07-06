#ifndef REID_MODULE_H
#define REID_MODULE_H

#include <stdint.h>
#include "rknn_detector.h"

#define MAX_PETS 10
#define FEATURE_DIM 128

// 宠物信息
typedef struct {
    char id[32];           // 宠物ID
    char name[64];         // 宠物名称
    float feature[FEATURE_DIM];  // Re-ID 特征向量
} PetInfo;

// 加载已注册宠物（从 /userdata/config/pets.json）
int reid_load_pets(const char *config_path);

// 识别宠物（返回宠物ID，未识别返回 NULL）
const char* reid_identify(const uint8_t *crop_image, int width, int height, float *confidence);

// 获取已注册宠物列表
int reid_get_pets(PetInfo *pets, int max_count);

// 释放资源
void reid_deinit();

#endif // REID_MODULE_H
