#include "food_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

static BowlROI g_bowl_roi = {320, 400, 180, 160};
static atomic_int g_current_level = FOOD_LEVEL_UNKNOWN;

int food_detector_init(const char *model_path) {
    if (!model_path) {
        return -1;
    }

    printf("加载余量检测模型: %s\n", model_path);

    // TODO: 实际的 RKNN 初始化（MobileNetV2）
    /*
    // 与 rknn_detector 类似，初始化 MobileNetV2 模型
    */

    return 0;
}

int food_detector_load_roi(const char *config_path) {
    if (!config_path) {
        return -1;
    }

    printf("加载碗ROI配置: %s\n", config_path);

    // TODO: 解析 JSON 配置
    /*
    FILE *fp = fopen(config_path, "r");
    if (!fp) {
        return -1;
    }

    // 读取并解析 JSON
    cJSON *root = cJSON_Parse(...);
    cJSON *bowl = cJSON_GetObjectItem(root, "bowl");
    g_bowl_roi.x = cJSON_GetObjectItem(bowl, "x")->valueint;
    g_bowl_roi.y = cJSON_GetObjectItem(bowl, "y")->valueint;
    g_bowl_roi.w = cJSON_GetObjectItem(bowl, "w")->valueint;
    g_bowl_roi.h = cJSON_GetObjectItem(bowl, "h")->valueint;
    cJSON_Delete(root);
    fclose(fp);
    */

    printf("碗ROI: (%d, %d, %d, %d)\n",
           g_bowl_roi.x, g_bowl_roi.y, g_bowl_roi.w, g_bowl_roi.h);

    return 0;
}

FoodLevel food_detector_check(const uint8_t *bowl_image) {
    if (!bowl_image) {
        return FOOD_LEVEL_UNKNOWN;
    }

    // TODO: 实际的推理
    /*
    // 1. 运行 MobileNetV2 推理
    rknn_input inputs[1];
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].size = 128 * 128 * 3;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].buf = (void*)bowl_image;

    rknn_inputs_set(food_ctx, 1, inputs);
    rknn_run(food_ctx, NULL);

    rknn_output outputs[1];
    outputs[0].want_float = 1;
    rknn_outputs_get(food_ctx, 1, outputs, NULL);

    // 2. 找到最大概率的类别
    float *probs = (float*)outputs[0].buf;
    int max_idx = 0;
    float max_prob = probs[0];
    for (int i = 1; i < 4; i++) {
        if (probs[i] > max_prob) {
            max_prob = probs[i];
            max_idx = i;
        }
    }

    rknn_outputs_release(food_ctx, 1, outputs);

    FoodLevel level = (FoodLevel)max_idx;
    atomic_store(&g_current_level, level);
    return level;
    */

    // 临时模拟
    FoodLevel level = FOOD_LEVEL_HALF;
    atomic_store(&g_current_level, level);
    return level;
}

BowlROI food_detector_get_roi() {
    return g_bowl_roi;
}

void food_detector_deinit() {
    // TODO: 清理 RKNN 资源
}

const char* food_level_to_string(FoodLevel level) {
    switch (level) {
        case FOOD_LEVEL_FULL: return "full";
        case FOOD_LEVEL_HALF: return "half";
        case FOOD_LEVEL_LOW: return "low";
        case FOOD_LEVEL_EMPTY: return "empty";
        default: return "unknown";
    }
}
