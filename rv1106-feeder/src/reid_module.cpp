#include "reid_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// 注意：需要 JSON 解析库（如 cJSON）
// #include "cJSON.h"

static PetInfo g_pets[MAX_PETS];
static int g_pet_count = 0;

// 计算余弦相似度
static float cosine_similarity(const float *a, const float *b, int dim) {
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (int i = 0; i < dim; i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    return dot / (sqrtf(norm_a) * sqrtf(norm_b));
}

int reid_load_pets(const char *config_path) {
    if (!config_path) {
        return -1;
    }

    printf("加载宠物配置: %s\n", config_path);

    // TODO: 实际的 JSON 解析
    /*
    FILE *fp = fopen(config_path, "r");
    if (!fp) {
        fprintf(stderr, "无法打开配置文件: %s\n", config_path);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *json_str = (char*)malloc(file_size + 1);
    fread(json_str, 1, file_size, fp);
    json_str[file_size] = '\0';
    fclose(fp);

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root) {
        fprintf(stderr, "JSON 解析失败\n");
        return -1;
    }

    cJSON *pets_array = cJSON_GetObjectItem(root, "pets");
    g_pet_count = cJSON_GetArraySize(pets_array);

    for (int i = 0; i < g_pet_count && i < MAX_PETS; i++) {
        cJSON *pet = cJSON_GetArrayItem(pets_array, i);

        cJSON *id = cJSON_GetObjectItem(pet, "id");
        cJSON *name = cJSON_GetObjectItem(pet, "name");
        cJSON *feature = cJSON_GetObjectItem(pet, "feature");

        strncpy(g_pets[i].id, id->valuestring, sizeof(g_pets[i].id) - 1);
        strncpy(g_pets[i].name, name->valuestring, sizeof(g_pets[i].name) - 1);

        // 解析特征向量
        for (int j = 0; j < FEATURE_DIM; j++) {
            cJSON *val = cJSON_GetArrayItem(feature, j);
            g_pets[i].feature[j] = (float)val->valuedouble;
        }
    }

    cJSON_Delete(root);
    */

    // 临时模拟数据
    g_pet_count = 2;
    strcpy(g_pets[0].id, "cat_001");
    strcpy(g_pets[0].name, "小花");
    strcpy(g_pets[1].id, "cat_002");
    strcpy(g_pets[1].name, "小白");

    printf("已加载 %d 只宠物\n", g_pet_count);
    return 0;
}

const char* reid_identify(const uint8_t *crop_image, int width, int height, float *confidence) {
    if (!crop_image || g_pet_count == 0) {
        return NULL;
    }

    // TODO: 实际的 Re-ID 推理
    /*
    // 1. 运行 Re-ID 模型提取特征
    float query_feature[FEATURE_DIM];
    rknn_reid_extract_feature(crop_image, width, height, query_feature);

    // 2. 与已注册宠物特征比对
    float max_similarity = 0.0f;
    int best_match = -1;

    for (int i = 0; i < g_pet_count; i++) {
        float sim = cosine_similarity(query_feature, g_pets[i].feature, FEATURE_DIM);
        if (sim > max_similarity) {
            max_similarity = sim;
            best_match = i;
        }
    }

    // 3. 判断阈值（例如 0.7）
    if (max_similarity > 0.7f) {
        if (confidence) *confidence = max_similarity;
        return g_pets[best_match].id;
    }
    */

    // 临时模拟
    if (confidence) *confidence = 0.85f;
    return g_pets[0].id;
}

int reid_get_pets(PetInfo *pets, int max_count) {
    if (!pets) {
        return g_pet_count;
    }

    int count = g_pet_count < max_count ? g_pet_count : max_count;
    memcpy(pets, g_pets, count * sizeof(PetInfo));
    return count;
}

void reid_deinit() {
    g_pet_count = 0;
}
