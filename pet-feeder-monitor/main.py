import cv2
import time
import os

from config import WINDOW_TITLE, LOG_ENABLED, LOG_DIR
from mqtt_client import MQTTClient
from video_stream import VideoStream
from overlay import draw


def main():
    mqtt_client = MQTTClient()
    stream = VideoStream()

    log_file = None
    if LOG_ENABLED:
        os.makedirs(LOG_DIR, exist_ok=True)
        log_path = os.path.join(LOG_DIR, f"{time.strftime('%Y%m%d_%H%M%S')}.log")
        log_file = open(log_path, "a")

    print("启动成功。按 [空格] 手动触发喂食，按 [Q] 退出。")

    while True:
        ret, frame = stream.read()
        if not ret:
            time.sleep(0.5)
            continue

        results = mqtt_client.get_results()
        frame = draw(frame, results)
        cv2.imshow(WINDOW_TITLE, frame)

        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            break
        elif key == ord(' '):
            mqtt_client.trigger_feed()
            ts = time.strftime("%H:%M:%S")
            print(f"[{ts}] 手动触发喂食")
            if log_file:
                log_file.write(f"{ts} MANUAL_FEED\n")
                log_file.flush()

        if log_file and results:
            log_file.write(f"{time.strftime('%H:%M:%S')} {results}\n")
            log_file.flush()

    stream.release()
    cv2.destroyAllWindows()
    mqtt_client.stop()
    if log_file:
        log_file.close()


if __name__ == "__main__":
    main()
