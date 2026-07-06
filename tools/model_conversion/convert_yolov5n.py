#!/usr/bin/env python3
"""
YOLOv5n 模型转换脚本
将 PyTorch .pt 模型转换为 RKNN .rknn 格式
适用平台: RV1106
"""

import os
import sys
import argparse
from rknn.api import RKNN

def export_onnx(pt_path, onnx_path, img_size=416):
    """导出 ONNX 模型"""
    print(f"[1/3] 导出 ONNX 模型: {pt_path} -> {onnx_path}")

    # 需要 YOLOv5 源码环境
    try:
        import torch
        sys.path.append('./yolov5')
        from models.experimental import attempt_load
        from models.yolo import Detect

        # 加载模型
        device = torch.device('cpu')
        model = attempt_load(pt_path, device=device)
        model.eval()

        # 修改最后一层，移除板端不兼容的后处理
        for m in model.modules():
            if isinstance(m, Detect):
                m.inplace = False
                m.onnx_dynamic = False
                m.export = True

        # 导出 ONNX
        dummy_input = torch.randn(1, 3, img_size, img_size, device=device)
        torch.onnx.export(
            model,
            dummy_input,
            onnx_path,
            opset_version=12,
            input_names=['images'],
            output_names=['output0', 'output1', 'output2'],
            dynamic_axes=None
        )

        print(f"✓ ONNX 导出成功: {onnx_path}")
        return True

    except Exception as e:
        print(f"✗ ONNX 导出失败: {e}")
        return False


def convert_rknn(onnx_path, rknn_path, quantize=True):
    """转换为 RKNN 格式"""
    print(f"[2/3] 转换为 RKNN 格式: {onnx_path} -> {rknn_path}")

    rknn = RKNN(verbose=True)

    # 配置
    print("配置 RKNN...")
    ret = rknn.config(
        mean_values=[[0, 0, 0]],
        std_values=[[255, 255, 255]],
        target_platform='rv1106',
        quantized_algorithm='normal' if quantize else 'none',
        quantized_dtype='asymmetric_quantized-8' if quantize else 'float16',
        optimization_level=3
    )

    if ret != 0:
        print("✗ 配置失败")
        return False

    # 加载 ONNX
    print("加载 ONNX 模型...")
    ret = rknn.load_onnx(model=onnx_path)
    if ret != 0:
        print("✗ 加载 ONNX 失败")
        return False

    # 构建模型
    print("构建 RKNN 模型...")
    ret = rknn.build(do_quantization=quantize)
    if ret != 0:
        print("✗ 构建失败")
        return False

    # 导出
    print("导出 RKNN...")
    ret = rknn.export_rknn(rknn_path)
    if ret != 0:
        print("✗ 导出失败")
        return False

    rknn.release()
    print(f"✓ RKNN 转换成功: {rknn_path}")
    return True


def test_inference(rknn_path, test_image=None):
    """测试推理"""
    print(f"[3/3] 测试推理: {rknn_path}")

    if not test_image or not os.path.exists(test_image):
        print("⊙ 跳过推理测试（未提供测试图片）")
        return True

    try:
        import cv2
        import numpy as np

        rknn = RKNN()
        ret = rknn.load_rknn(rknn_path)
        if ret != 0:
            print("✗ 加载 RKNN 失败")
            return False

        ret = rknn.init_runtime()
        if ret != 0:
            print("✗ 初始化运行时失败")
            return False

        # 读取测试图片
        img = cv2.imread(test_image)
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        img = cv2.resize(img, (416, 416))

        # 推理
        outputs = rknn.inference(inputs=[img])

        print(f"✓ 推理成功")
        print(f"  输出数量: {len(outputs)}")
        for i, output in enumerate(outputs):
            print(f"  输出[{i}] shape: {output.shape}")

        rknn.release()
        return True

    except Exception as e:
        print(f"✗ 推理测试失败: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(description='YOLOv5n 转 RKNN 模型')
    parser.add_argument('--weight', type=str, required=True, help='YOLOv5 .pt 模型路径')
    parser.add_argument('--output', type=str, default='yolov5n.rknn', help='输出 RKNN 文件路径')
    parser.add_argument('--img-size', type=int, default=416, help='输入图像尺寸')
    parser.add_argument('--no-quantize', action='store_true', help='不进行量化（使用 float16）')
    parser.add_argument('--test-image', type=str, help='测试图片路径')
    parser.add_argument('--skip-onnx', action='store_true', help='跳过 ONNX 导出（假设已存在）')

    args = parser.parse_args()

    print("=" * 60)
    print("YOLOv5n -> RKNN 模型转换")
    print("=" * 60)
    print(f"输入模型: {args.weight}")
    print(f"输出路径: {args.output}")
    print(f"图像尺寸: {args.img_size}x{args.img_size}")
    print(f"量化模式: {'INT8' if not args.no_quantize else 'FP16'}")
    print("=" * 60)

    # 生成临时 ONNX 路径
    onnx_path = args.weight.replace('.pt', '.onnx')

    # 步骤1: 导出 ONNX（如果不跳过）
    if not args.skip_onnx:
        if not export_onnx(args.weight, onnx_path, args.img_size):
            print("\n转换失败！")
            return 1
    else:
        if not os.path.exists(onnx_path):
            print(f"✗ ONNX 文件不存在: {onnx_path}")
            return 1
        print(f"[1/3] 使用已存在的 ONNX: {onnx_path}")

    # 步骤2: 转换 RKNN
    if not convert_rknn(onnx_path, args.output, quantize=not args.no_quantize):
        print("\n转换失败！")
        return 1

    # 步骤3: 测试推理
    if not test_inference(args.output, args.test_image):
        print("\n推理测试失败！")
        return 1

    print("\n" + "=" * 60)
    print("✓ 转换完成！")
    print(f"  RKNN 模型: {args.output}")
    print(f"  模型大小: {os.path.getsize(args.output) / 1024 / 1024:.2f} MB")
    print("=" * 60)

    return 0


if __name__ == '__main__':
    sys.exit(main())
