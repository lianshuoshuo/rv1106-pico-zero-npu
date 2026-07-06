# 模型转换工具

将 PyTorch 模型转换为 RV1106 可用的 RKNN 格式。

## 环境准备

### 1. 安装 RKNN Toolkit 2

```bash
# 下载 RKNN Toolkit 2（RV1106 版本）
# https://github.com/rockchip-linux/rknn-toolkit2

# 安装依赖
pip install rknn-toolkit2
pip install torch torchvision
```

### 2. 安装模型依赖库

```bash
# YOLOv5
git clone https://github.com/ultralytics/yolov5.git
cd yolov5
pip install -r requirements.txt

# Re-ID (torchreid)
pip install torchreid

# MobileNetV2（torchvision 已包含）
```

## 使用方法

### YOLOv5n 转换

```bash
python convert_yolov5n.py \
    --weight yolov5n.pt \
    --output ../../../rv1106-feeder/models/yolov5n.rknn \
    --img-size 416 \
    --test-image test_cat.jpg
```

**参数说明：**
- `--weight`: YOLOv5 PyTorch 模型路径
- `--output`: 输出 RKNN 文件路径
- `--img-size`: 输入图像尺寸（默认416）
- `--no-quantize`: 跳过量化（使用 FP16，模型更大但精度更高）
- `--test-image`: 测试图片路径（可选）
- `--skip-onnx`: 跳过 ONNX 导出（假设已存在）

### Re-ID OSNet 转换

```bash
python convert_reid.py \
    --model osnet_x0_25.pth \
    --output ../../../rv1106-feeder/models/reid.rknn \
    --input-size 128 \
    --test-image test_pet.jpg
```

**参数说明：**
- `--model`: OSNet PyTorch 模型路径
- `--output`: 输出 RKNN 文件路径
- `--input-size`: 输入图像尺寸（默认128）
- `--no-quantize`: 跳过量化
- `--test-image`: 测试图片路径（可选）

### MobileNetV2 余量检测转换

```bash
python convert_mobilenet.py \
    --model food_classifier.pth \
    --output ../../../rv1106-feeder/models/mobilenet.rknn \
    --num-classes 4 \
    --input-size 128 \
    --test-image test_bowl.jpg
```

**参数说明：**
- `--model`: MobileNetV2 PyTorch 模型路径
- `--output`: 输出 RKNN 文件路径
- `--num-classes`: 分类数量（默认4：full/half/low/empty）
- `--input-size`: 输入图像尺寸（默认128）
- `--no-quantize`: 跳过量化
- `--test-image`: 测试图片路径（可选）

## 转换流程

所有转换脚本都遵循三步流程：

1. **导出 ONNX** — 将 PyTorch 模型转为 ONNX 格式
2. **转换 RKNN** — 使用 RKNN Toolkit 2 转换并量化
3. **测试推理** — 在 PC 端验证模型可用性

## 预训练模型下载

### YOLOv5n

```bash
wget https://github.com/ultralytics/yolov5/releases/download/v7.0/yolov5n.pt
```

### OSNet-x0.25

```bash
# 下载预训练权重
wget https://drive.google.com/uc?id=1vduhq5DpN2q1g4fYEZfPI17MJeh9qyrA -O osnet_x0_25.pth
```

### MobileNetV2

需要自己训练食物余量分类模型：

```python
from torchvision import models
import torch.nn as nn

# 构建模型
model = models.mobilenet_v2(pretrained=True)
model.classifier[1] = nn.Linear(model.last_channel, 4)

# 训练模型...
# 保存权重
torch.save(model.state_dict(), 'food_classifier.pth')
```

## 模型训练

### 余量检测数据集

需要采集训练数据（样机出来后）：

```
dataset/
├── train/
│   ├── full/    # 满碗（200-300张）
│   ├── half/    # 半碗
│   ├── low/     # 余量低
│   └── empty/   # 空碗
└── val/
    ├── full/
    ├── half/
    ├── low/
    └── empty/
```

训练脚本：

```python
import torch
import torch.nn as nn
from torchvision import models, datasets, transforms
from torch.utils.data import DataLoader

# 数据增强
transform_train = transforms.Compose([
    transforms.Resize((128, 128)),
    transforms.RandomHorizontalFlip(),
    transforms.RandomRotation(10),
    transforms.ColorJitter(0.2, 0.2, 0.2),
    transforms.ToTensor(),
    transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
])

# 加载数据
train_dataset = datasets.ImageFolder('dataset/train', transform=transform_train)
train_loader = DataLoader(train_dataset, batch_size=32, shuffle=True)

# 构建模型
model = models.mobilenet_v2(pretrained=True)
model.classifier[1] = nn.Linear(model.last_channel, 4)

# 训练...
optimizer = torch.optim.Adam(model.parameters(), lr=0.001)
criterion = nn.CrossEntropyLoss()

for epoch in range(20):
    for images, labels in train_loader:
        outputs = model(images)
        loss = criterion(outputs, labels)
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()

# 保存
torch.save(model.state_dict(), 'food_classifier.pth')
```

## 常见问题

### Q1: RKNN Toolkit 2 安装失败

```bash
# 确保使用 Python 3.8-3.10
python --version

# 升级 pip
pip install --upgrade pip

# 使用国内源
pip install rknn-toolkit2 -i https://pypi.tuna.tsinghua.edu.cn/simple
```

### Q2: 转换后模型精度下降

- 使用 `--no-quantize` 跳过量化，改用 FP16
- 检查 mean/std 归一化参数是否正确
- 增加量化数据集（如果使用量化）

### Q3: 板端推理失败

- 确认 RV1106 RKNN Runtime 版本与 Toolkit 版本匹配
- 检查模型输入输出格式（NHWC vs NCHW）
- 查看板端日志：`dmesg | grep rknn`

## 输出文件

转换成功后，将 `.rknn` 文件复制到设备：

```bash
scp yolov5n.rknn root@192.168.1.100:/userdata/models/
scp reid.rknn root@192.168.1.100:/userdata/models/
scp mobilenet.rknn root@192.168.1.100:/userdata/models/
```

## 相关文档

- [RKNN神经网络推理指南](../../docs/RKNN神经网络推理指南.md)
- [rv1106-feeder README](../../rv1106-feeder/README.md)
