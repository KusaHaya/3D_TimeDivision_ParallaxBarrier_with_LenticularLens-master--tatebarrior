import math
import socket
import struct
import unittest

import numpy as np
import pyrealsense2 as rs
import realsense_tracking_server as tracking


class TrackingTests(unittest.TestCase):
    def test_axes_and_eye_order(self):
        intr = rs.intrinsics()
        intr.width, intr.height = 640, 480
        intr.fx = intr.fy = 600
        intr.ppx, intr.ppy = 320, 240
        intr.model = rs.distortion.none
        depth = np.full((480, 640), 1000, np.uint16)
        pair = tracking.eye_coordinates(depth, ((301, 220), (339, 220)), .001, intr)
        self.assertIsNotNone(pair)
        self.assertGreater(pair[0], 0)  # Image-left maps to Kinect +X.
        self.assertLess(pair[3], 0)
        self.assertGreater(pair[1], 0)  # Image-up maps to Kinect +Y.
        self.assertAlmostEqual(pair[2], 1.0)
        self.assertIsNone(tracking.eye_coordinates(depth, ((100, 220), (500, 220)), .001, intr))

    def test_depth_holes_and_outlier(self):
        depth = np.full((9, 9), 1000, np.uint16)
        depth[4, 4] = 3500
        self.assertAlmostEqual(tracking.median_depth_metres(depth, (4, 4), .001), 1.)
        depth[:6] = 0
        self.assertIsNone(tracking.median_depth_metres(depth, (4, 4), .001))

    def test_wire_packet_and_loss(self):
        sender, receiver = socket.socketpair()
        receiver.settimeout(1)
        try:
            values = [0.03, 0.02, 1., -0.03, 0.02, 1.]
            self.assertIs(tracking.send_coordinates(sender, values), sender)
            packet = receiver.recv(24)
            self.assertEqual(len(packet), 24)
            np.testing.assert_allclose(struct.unpack('<6f', packet), values)
            tracking.send_coordinates(sender, None)
            self.assertTrue(all(math.isnan(v) for v in struct.unpack('<6f', receiver.recv(24))))
        finally:
            sender.close()
            receiver.close()

    def test_multiple_people_are_not_selected(self):
        class Faces:
            def detectMultiScale(self, *args, **kwargs):
                return [(0, 0, 100, 100), (110, 0, 100, 100)]
        self.assertEqual(tracking.detect_eyes(np.zeros((240, 320, 3), np.uint8), Faces(), None), (None, None))

    def test_models_load(self):
        for name in ('haarcascade_frontalface_default.xml', 'haarcascade_eye_tree_eyeglasses.xml'):
            self.assertFalse(tracking.make_classifier(name).empty())


if __name__ == '__main__':
    unittest.main()
