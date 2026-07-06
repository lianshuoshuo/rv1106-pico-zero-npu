#ifndef RKNN_DETECTOR_H
#define RKNN_DETECTOR_H

#include <stdint.h>

#define MAX_DETECTIONS 10

// 检测框
typedef struct {
    float x, y, w, h;    // 归一化坐标 [0, 1]
    float confidence;
    int class_id;        // 0: cat, 1: dog
    char class_name[16];
} DetectBox;

// 检测结果
typedef struct {
    DetectBox boxes[MAX_DETECTIONS];
    int count;
} DetectResult;

// 初始化 YOLOv5n 模型
int rknn_detector_init(const char *model_path);

// 运行推理（输入 416x416 RGB）
int rknn_detector_run(const uint8_t *rgb_data, DetectResult *result);

// 释放资源
void rknn_detector_deinit();

#endif // RKNN_DETECTOR_H
