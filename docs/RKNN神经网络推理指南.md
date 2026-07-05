# RKNN 神经网络推理指南

> 更新日期：2026-07-04  
> 芯片：RV1106G3，NPU 算力 1.0 TOPS（int4/int8/int16）

---

## 一、概述

整体流程：PC 端（Ubuntu x86）转换模型 → 板端用 C API 部署推理。

```
训练框架（PyTorch/TF） → ONNX → RKNN（PC 转换） → 板端 C API 推理
```

---

## 二、PC 端环境：RKNN-Toolkit2

### 2.1 Conda 安装（推荐）

```bash
conda create -n RKNN-Toolkit2 python=3.9
conda activate RKNN-Toolkit2

git clone https://github.com/airockchip/rknn-toolkit2.git

pip install -r rknn-toolkit2/packages/x86_64/requirements_cp39-2.3.2.txt \
    -i https://pypi.mirrors.ustc.edu.cn/simple/

pip install rknn-toolkit2/packages/x86_64/rknn_toolkit2-2.3.2-cp39-cp39-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
```

验证：
```python
python3 -c "from rknn.api import RKNN; print('OK')"
```

### 2.2 本地安装（Ubuntu 22.04）

```bash
git clone https://github.com/airockchip/rknn-toolkit2
sudo apt-get install -y python3 python3-dev python3-pip libxslt1-dev \
    zlib1g-dev libprotobuf-dev gcc

pip3 install -r rknn-toolkit2/packages/x86_64/requirements_cp310-2.3.2.txt
pip3 install rknn-toolkit2/packages/x86_64/rknn_toolkit2-2.3.2-cp310-cp310-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
```

---

## 三、ONNX 模型导出

### 3.1 RetinaFace（人脸检测）

```python
import torch
from nets.retinaface import RetinaFace
from utils.config import cfg_mnet

model = RetinaFace(cfg=cfg_mnet, pretrained=False)
model.load_state_dict(torch.load('model_data/Retinaface_mobilenet0.25.pth', map_location='cpu'), strict=False)
model.eval()
example = torch.rand(1, 3, 640, 640)
torch.onnx.export(model, (example,), 'model_data/retinaface.onnx', opset_version=9)
```

### 3.2 Facenet（人脸特征提取）

> 注意：模型末尾的 `ReduceL2` 算子（`F.normalize`）RKNPU 不支持，需注释掉后重新导出，标准化改在 CPU 端完成。

```python
import torch
from nets.facenet import Facenet

model = Facenet(backbone="mobilenet", mode="predict", pretrained=True)
model.load_state_dict(torch.load('model_data/facenet_mobilenet.pth', map_location='cpu'), strict=False)
example = torch.rand(1, 3, 160, 160)
torch.onnx.export(model, example, 'model_data/facenet.onnx', opset_version=9)
```

### 3.3 YOLOv5（目标检测）

必须加 `--rknpu` 参数，移除 RKNPU 不兼容的后处理层：

```bash
python export.py --rknpu --weight yolov5s.pt
# 生成 yolov5s.onnx 和 RK_anchors.txt
```

---

## 四、ONNX → RKNN 模型转换

### 4.1 使用 Luckfox 官方示例

```bash
git clone https://github.com/LuckfoxTECH/luckfox_pico_rknn_example.git
cd luckfox_pico_rknn_example/scripts/luckfox_onnx_to_rknn/convert
conda activate RKNN-Toolkit2

python convert.py ../model/retinaface.onnx \
    ../dataset/retinaface_dataset.txt \
    ../model/retinaface.rknn \
    Retinaface
# 参数：<onnx路径> <量化数据集txt> <输出rknn路径> <模型类型>
```

### 4.2 预处理配置（必须与训练一致）

以 RetinaFace 为例，训练时各通道减均值：

```python
rknn.config(
    mean_values=[[104, 117, 123]],   # 与训练预处理一致
    std_values=[[1, 1, 1]],
    target_platform='rv1106',
    quantized_algorithm="normal",
    quant_img_RGB2BGR=True
)
```

### 4.3 使用 rknn_model_zoo

```bash
git clone https://github.com/airockchip/rknn_model_zoo.git
conda activate RKNN-Toolkit2
cd rknn_model_zoo/examples/yolov5/python
python3 convert.py ../model/yolov5s.onnx rv1106
```

---

## 五、PC 端模型验证（软件模拟器）

```python
from rknn.api import RKNN

rknn = RKNN()
rknn.config(mean_values=[[0,0,0]], std_values=[[128,128,128]],
            target_platform='rv1103', quantized_algorithm="normal")
rknn.load_onnx(model=model_path)
rknn.build(do_quantization=True, dataset=DATASET_PATH)
rknn.init_runtime()   # 不传 target 参数 → 软件模拟器
# ... 推理 ...
rknn.release()
```

> 模拟器输出为浮点（未量化），板端输出为 int8，对比时需反量化。

---

## 六、板端 C API 推理

### 6.1 初始化

```c
// 通用方式
rknn_init(&ctx, model_path, 0, 0, NULL);

// 多模型共享内存（节省资源）
rknn_init(&ctx_a, model_path_a, 0, RKNN_FLAG_MEM_ALLOC_OUTSIDE, NULL);
rknn_query(ctx_a, RKNN_QUERY_MEM_SIZE, &mem_size_a, sizeof(mem_size_a));
internal_mem = rknn_create_mem(ctx_a, max_internal_size);
rknn_set_internal_mem(ctx_a, internal_mem);
```

### 6.2 查询输入输出参数

```c
rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
rknn_query(ctx, RKNN_QUERY_NATIVE_INPUT_ATTR,  &input_attrs[i],  sizeof(rknn_tensor_attr));
rknn_query(ctx, RKNN_QUERY_NATIVE_OUTPUT_ATTR, &output_attrs[i], sizeof(rknn_tensor_attr));
```

### 6.3 内存申请与绑定

```c
input_mems[i]  = rknn_create_mem(ctx, input_attrs[i].size_with_stride);
output_mems[i] = rknn_create_mem(ctx, output_attrs[i].size_with_stride);
rknn_set_io_mem(ctx, input_mems[i],  &input_attrs[i]);
rknn_set_io_mem(ctx, output_mems[i], &output_attrs[i]);
```

### 6.4 输入图像处理（BGR → RGB）

opencv-mobile 采集默认为 BGR，需手动转 RGB 后写入：

```c
// 手动转换
src_image[(y*w+x)*3+0] = pixel[2];  // R
src_image[(y*w+x)*3+1] = pixel[1];  // G
src_image[(y*w+x)*3+2] = pixel[0];  // B
memcpy(input_mems[0]->virt_addr, src_image, w * h * channels);
```

### 6.5 推理（零拷贝接口）

```c
rknn_run(ctx, nullptr);
```

### 6.6 输出反量化（int8 → float）

```c
float deqnt = ((float)qnt - (float)zp) * scale;
```

Facenet 额外需在 CPU 端补充 L2 标准化：

```c
float sum = 0;
for (int i = 0; i < 128; i++) sum += out_fp32[i] * out_fp32[i];
float norm = sqrt(sum);
for (int i = 0; i < 128; i++) out_fp32[i] /= norm;
```

---

## 七、编译与部署

### 7.1 Luckfox 官方示例编译

```bash
export LUCKFOX_SDK_PATH=<SDK 绝对路径>
./build.sh
# 选择：1=retinaface_facenet  2=spidev 版  3=yolov5
```

### 7.2 rknn_model_zoo 交叉编译

```bash
export GCC_COMPILER=<SDK>/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf

chmod +x ./build-linux.sh
./build-linux.sh -t rv1106 -a armv7l -d yolov5
# 产物：install/rv1106_linux_armv7l/rknn_yolov5_demo/
```

### 7.3 板端运行

```bash
# 先释放摄像头占用
RkLunch-stop.sh
# 或
killall rkipc

# 人脸识别
./luckfox_pico_retinaface_facenet ./model/RetinaFace.rknn ./model/mobilefacenet.rknn ./model/test.jpg

# 目标检测
./luckfox_pico_yolov5 ./model/yolov5.rknn

# rknn_model_zoo yolov5
./rknn_yolov5_demo model/yolov5.rknn model/bus.jpg
```

---

## 八、关键约束

| 约束项 | 说明 |
|--------|------|
| 输入/输出类型 | 仅支持 int8，float 类型会导致数据错误 |
| Tensor 维度 | 仅支持 4 维 |
| 推理 API | 板端仅支持 C API，不支持 Python |
| 不兼容算子 | ReduceL2、Layer Normalization 等须裁剪到 CPU |
| 量化反算 | `(qnt - zp) * scale` |

---

## 九、常见问题

| 问题 | 解决方案 |
|------|---------|
| 找不到摄像头 | `killall rkipc` 或 `RkLunch-stop.sh` |
| 模型转换失败 | 确认已激活 `RKNN-Toolkit2` 环境 |
| pip 下载慢 | 使用镜像：`-i https://pypi.mirrors.ustc.edu.cn/simple/` |
| 输出数据异常 | 检查 mean/std 是否与训练一致；确认输出类型为 int8 |
