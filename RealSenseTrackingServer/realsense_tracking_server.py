"""RealSense eye-position server compatible with ProjectPB's Kinect TCP protocol.

The renderer expects six little-endian float32 values on 127.0.0.1:30000:
image-left eye XYZ followed by image-right eye XYZ, in metres and Kinect axes.
Six NaNs indicate tracking loss to the updated renderer.
"""

from __future__ import annotations

import argparse
import socket
import struct
from dataclasses import dataclass
from typing import Optional, Sequence, Tuple

import cv2
import numpy as np
import pyrealsense2 as rs


Point3 = Tuple[float, float, float]
EyePair = Tuple[Tuple[int, int], Tuple[int, int]]


@dataclass
class ExponentialSmoother:
    alpha: float
    value: Optional[np.ndarray] = None

    def update(self, sample: Sequence[float]) -> np.ndarray:
        current = np.asarray(sample, dtype=np.float32)
        if self.value is None:
            self.value = current
        else:
            self.value = self.alpha * current + (1.0 - self.alpha) * self.value
        return self.value


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Send RealSense eye coordinates to ProjectPB over TCP."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=30000)
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument("--fps", type=int, default=30)
    parser.add_argument(
        "--smoothing",
        type=float,
        default=0.35,
        help="EMA weight for a new sample (0 < value <= 1).",
    )
    parser.add_argument("--no-preview", action="store_true")
    return parser.parse_args()


def make_classifier(filename: str) -> cv2.CascadeClassifier:
    classifier = cv2.CascadeClassifier(cv2.data.haarcascades + filename)
    if classifier.empty():
        raise RuntimeError(f"OpenCV cascade could not be loaded: {filename}")
    return classifier


def detect_eyes(
    bgr: np.ndarray,
    face_classifier: cv2.CascadeClassifier,
    eye_classifier: cv2.CascadeClassifier,
) -> Tuple[Optional[EyePair], Optional[Tuple[int, int, int, int]]]:
    gray = cv2.cvtColor(bgr, cv2.COLOR_BGR2GRAY)
    gray = cv2.equalizeHist(gray)
    faces = face_classifier.detectMultiScale(
        gray, scaleFactor=1.1, minNeighbors=5, minSize=(100, 100)
    )
    if len(faces) != 1:
        return None, None

    x, y, w, h = max(faces, key=lambda rect: int(rect[2]) * int(rect[3]))
    upper_h = max(1, int(h * 0.62))
    roi = gray[y : y + upper_h, x : x + w]
    detections = eye_classifier.detectMultiScale(
        roi,
        scaleFactor=1.08,
        minNeighbors=6,
        minSize=(max(18, w // 10), max(12, h // 12)),
    )

    candidates = []
    for ex, ey, ew, eh in detections:
        center = (x + ex + ew // 2, y + ey + eh // 2)
        candidates.append((center, int(ew) * int(eh)))

    best_pair: Optional[EyePair] = None
    best_score = -1.0
    for i in range(len(candidates)):
        for j in range(i + 1, len(candidates)):
            first, first_area = candidates[i]
            second, second_area = candidates[j]
            left, right = sorted((first, second), key=lambda p: p[0])
            separation = right[0] - left[0]
            vertical_difference = abs(right[1] - left[1])
            if separation < 0.20 * w or separation > 0.75 * w:
                continue
            if vertical_difference > 0.20 * h:
                continue
            score = first_area + second_area + 10.0 * separation - 15.0 * vertical_difference
            if score > best_score:
                best_score = score
                best_pair = (left, right)

    return best_pair, (int(x), int(y), int(w), int(h))


def median_depth_metres(
    depth_image: np.ndarray,
    pixel: Tuple[int, int],
    depth_scale: float,
    radius: int = 4,
) -> Optional[float]:
    x, y = pixel
    height, width = depth_image.shape
    x0, x1 = max(0, x - radius), min(width, x + radius + 1)
    y0, y1 = max(0, y - radius), min(height, y + radius + 1)
    values = depth_image[y0:y1, x0:x1].astype(np.float32) * depth_scale
    valid = values[(values > 0.10) & (values < 4.0)]
    if valid.size < max(9, values.size // 2):
        return None
    return float(np.median(valid))


def kinect_compatible_point(
    intrinsics: rs.intrinsics,
    pixel: Tuple[int, int],
    depth_metres: float,
) -> Point3:
    # RealSense: +X right, +Y down, +Z forward.
    # Kinect camera space used by the original server: +X left, +Y up, +Z forward.
    x_rs, y_rs, z_rs = rs.rs2_deproject_pixel_to_point(
        intrinsics, [float(pixel[0]), float(pixel[1])], depth_metres
    )
    return -float(x_rs), -float(y_rs), float(z_rs)


def open_listener(host: str, port: int) -> socket.socket:
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind((host, port))
    listener.listen(1)
    listener.setblocking(False)
    print(f"Waiting for ProjectPB on {host}:{port} ...")
    return listener


def try_accept(listener: socket.socket) -> Optional[socket.socket]:
    try:
        client, address = listener.accept()
    except BlockingIOError:
        return None
    client.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    client.settimeout(0.1)  # A stalled renderer must not freeze camera acquisition.
    print(f"ProjectPB connected from {address[0]}:{address[1]}")
    return client


def eye_coordinates(depth_image, eye_pixels, depth_scale, intrinsics):
    """Return a valid image-left/image-right pair, or None (no stale estimates)."""
    if eye_pixels is None:
        return None
    points = []
    for pixel in eye_pixels:
        depth = median_depth_metres(depth_image, pixel, depth_scale)
        if depth is None:
            return None
        points.extend(kinect_compatible_point(intrinsics, pixel, depth))
    result = np.asarray(points, dtype=np.float32)
    separation = np.linalg.norm(result[:3] - result[3:])
    # Reject false eye pairs and depth samples on the background.
    if not np.all(np.isfinite(result)) or not 0.035 <= separation <= 0.09:
        return None
    return result


def send_coordinates(client, coordinates):
    if client is None:
        return None
    values = coordinates if coordinates is not None else [float("nan")] * 6
    try:
        client.sendall(struct.pack("<6f", *values))
        return client
    except OSError:
        # A partial timed-out packet cannot be reused: discard the connection.
        client.close()
        print("ProjectPB disconnected; waiting for reconnection ...")
        return None


def run(args: argparse.Namespace) -> None:
    if not 0.0 < args.smoothing <= 1.0:
        raise ValueError("--smoothing must be greater than 0 and at most 1")

    face_classifier = make_classifier("haarcascade_frontalface_default.xml")
    eye_classifier = make_classifier("haarcascade_eye_tree_eyeglasses.xml")
    smoother = ExponentialSmoother(args.smoothing)
    listener = open_listener(args.host, args.port)
    client: Optional[socket.socket] = None

    pipeline = rs.pipeline()
    config = rs.config()
    config.enable_stream(rs.stream.depth, args.width, args.height, rs.format.z16, args.fps)
    config.enable_stream(rs.stream.color, args.width, args.height, rs.format.bgr8, args.fps)
    started = False
    try:
        profile = pipeline.start(config)
        started = True
        depth_scale = float(profile.get_device().first_depth_sensor().get_depth_scale())
        align_to_color = rs.align(rs.stream.color)
        print("RealSense started. Press Q in the preview window to quit.")
        while True:
            if client is None:
                client = try_accept(listener)

            frames = align_to_color.process(pipeline.wait_for_frames())
            depth_frame = frames.get_depth_frame()
            color_frame = frames.get_color_frame()
            if not depth_frame or not color_frame:
                continue

            color_image = np.asanyarray(color_frame.get_data())
            depth_image = np.asanyarray(depth_frame.get_data())
            eye_pixels, face = detect_eyes(color_image, face_classifier, eye_classifier)

            if face is not None and not args.no_preview:
                x, y, w, h = face
                cv2.rectangle(color_image, (x, y), (x + w, y + h), (0, 180, 0), 2)

            intrinsics = depth_frame.profile.as_video_stream_profile().intrinsics
            measured = eye_coordinates(depth_image, eye_pixels, depth_scale, intrinsics)
            coordinates = None
            if measured is not None:
                coordinates = smoother.update(measured)
                if not args.no_preview:
                    for point in eye_pixels:
                        cv2.circle(color_image, point, 5, (0, 0, 255), -1)
            else:
                smoother.value = None
            client = send_coordinates(client, coordinates)

            if not args.no_preview:
                state = "tracking" if coordinates is not None else "searching"
                cv2.putText(
                    color_image,
                    state,
                    (12, 30),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.8,
                    (0, 255, 0) if state == "tracking" else (0, 180, 255),
                    2,
                )
                cv2.imshow("RealSense Tracking Server", color_image)
                if cv2.waitKey(1) & 0xFF in (ord("q"), 27):
                    break
                if cv2.getWindowProperty("RealSense Tracking Server", cv2.WND_PROP_VISIBLE) < 1:
                    break
    finally:
        if client is not None:
            client.close()
        listener.close()
        if started:
            pipeline.stop()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    run(parse_args())
