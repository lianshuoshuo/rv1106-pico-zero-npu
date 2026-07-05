"""
UI 预览脚本：用模拟数据生成截图，无需硬件。
"""
import cv2
import numpy as np
from overlay import draw

# 模拟一帧背景（灰色+简单场景感）
frame = np.zeros((720, 1280, 3), dtype=np.uint8)
frame[:] = (45, 42, 40)  # 深灰背景

# 画出"地板"和"碗"的示意
cv2.rectangle(frame, (0, 500), (1280, 720), (60, 55, 50), -1)
cv2.ellipse(frame, (640, 560), (120, 40), 0, 0, 360, (80, 75, 70), -1)
cv2.putText(frame, "bowl area", (595, 565),
            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (100, 100, 100), 1)

# 模拟推理结果
mock_results = {
    "ts": 1720000000,
    "pets": [
        {
            "id": "cat_01",
            "confidence": 0.92,
            "box": [480, 340, 200, 180],
            "eating": True
        },
        {
            "id": "cat_02",
            "confidence": 0.78,
            "box": [820, 310, 160, 160],
            "eating": False
        }
    ],
    "food_level": "half"
}

frame = draw(frame, mock_results)

# 加标题
cv2.putText(frame, "Pet Feeder Monitor - DEMO", (460, 680),
            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (150, 150, 150), 1)

cv2.imwrite("preview_demo.png", frame)
print("已保存：preview_demo.png")
