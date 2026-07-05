import cv2
import time

COLORS = {
    "eating":   (0, 255, 0),    # 绿色：进食中
    "detected": (0, 165, 255),  # 橙色：检测到但未进食
    "food_low": (0, 0, 255),    # 红色：食物不足
    "food_ok":  (255, 255, 255) # 白色：食物充足
}


def draw(frame, results):
    for pet in results.get("pets", []):
        x, y, w, h = pet["box"]
        color = COLORS["eating"] if pet.get("eating") else COLORS["detected"]
        cv2.rectangle(frame, (x, y), (x + w, y + h), color, 2)
        label = f"{pet['id']} {pet['confidence']:.0%}"
        cv2.putText(frame, label, (x, y - 8),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)

    level = results.get("food_level", "--")
    color = COLORS["food_low"] if level in ("low", "empty") else COLORS["food_ok"]
    cv2.putText(frame, f"Food: {level}", (10, 35),
                cv2.FONT_HERSHEY_SIMPLEX, 1.0, color, 2)

    cv2.putText(frame, time.strftime("%H:%M:%S"), (10, frame.shape[0] - 10),
                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200, 200, 200), 1)
    return frame
