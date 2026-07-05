import cv2
from config import RTSP_URL


class VideoStream:
    def __init__(self, url=RTSP_URL):
        self.url = url
        self.cap = self._open()

    def _open(self):
        cap = cv2.VideoCapture(self.url)
        if not cap.isOpened():
            print(f"[VideoStream] 无法打开流: {self.url}")
        return cap

    def read(self):
        ret, frame = self.cap.read()
        if not ret:
            print("[VideoStream] 读帧失败，尝试重连...")
            self.cap.release()
            self.cap = self._open()
            ret, frame = self.cap.read()
        return ret, frame

    def release(self):
        self.cap.release()
