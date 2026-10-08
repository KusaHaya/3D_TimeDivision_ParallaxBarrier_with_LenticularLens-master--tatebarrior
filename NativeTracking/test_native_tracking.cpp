#include "../ProjectPB/RealSenseTracker.h"
#include "../ProjectPB/RealSenseTrackingMath.h"
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <stdexcept>

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
int main(int argc, char** argv) {
    try {
        cv::Mat depth(480, 640, CV_16UC1, cv::Scalar(1000));
        rs2_intrinsics intrinsics{};
        intrinsics.width = 640; intrinsics.height = 480;
        intrinsics.fx = intrinsics.fy = 600;
        intrinsics.ppx = 320; intrinsics.ppy = 240;
        intrinsics.model = RS2_DISTORTION_NONE;
        std::array<float, 6> coords;
        Check(tracking::EyeCoordinates(depth, {{{301, 220}, {339, 220}}}, .001f, intrinsics, coords), "Valid pair rejected");
        Check(coords[0] > 0 && coords[3] < 0 && coords[1] > 0 && std::abs(coords[2] - 1.f) < 1e-6, "Axes or units incorrect");
        Check(!tracking::EyeCoordinates(depth, {{{100, 220}, {500, 220}}}, .001f, intrinsics, coords), "Impossible eye pair accepted");
        depth.setTo(0);
        float metres;
        Check(!tracking::MedianDepth(depth, {0, 0}, .001f, metres), "Missing depth accepted");
        depth.setTo(1000);
        depth.at<unsigned short>(240, 320) = 3500;
        Check(tracking::MedianDepth(depth, {320, 240}, .001f, metres) && std::abs(metres - 1.f) < 1e-6, "Median failed");
        Check(!tracking::MedianDepth(depth, {-1, 240}, .001f, metres), "Invalid pixel accepted");
        tracking::LatestSample sample;
        const auto now = tracking::LatestSample::Clock::now();
        const std::array<float, 6> first{{.03f, .02f, 1.f, -.03f, .02f, 1.f}};
        std::array<float, 6> result;
        Check(!sample.Read(result, now), "Uninitialized sample accepted");
        sample.Update(first, now);
        Check(sample.Read(result, now) && result == first, "First measurement altered");
        Check(sample.Read(result, now + std::chrono::seconds(10)) && result == first, "Detection gap did not hold last position");
        auto outlier = first; outlier[0] += 1.f;
        Check(!sample.TryUpdate(outlier, now + std::chrono::milliseconds(33)), "Discontinuous sample accepted");
        Check(sample.Read(result, now) && result == first, "Rejected sample changed held position");
        sample.Invalidate();
        Check(!sample.Read(result, now), "Lost result accepted");
        auto moved = first;
        moved[0] += .1f;
        sample.Update(moved, now + std::chrono::seconds(1));
        Check(sample.Read(result, now + std::chrono::seconds(1)) && result == moved, "Reacquisition blended stale data");
        RealSenseTracker tracker;
        tracker.Stop();
        Check(!tracker.Poll(result), "Stopped tracker reported valid data");
        if (argc > 1) {
            const std::string directory(argv[1]);
            cv::CascadeClassifier face(directory + "/haarcascade_frontalface_default.xml");
            cv::CascadeClassifier eyes(directory + "/haarcascade_eye_tree_eyeglasses.xml");
            Check(!face.empty() && !eyes.empty(), "Bundled models failed to load");
        }
        std::cout << "PASS: native axes, depth, eye pair validation, held/outlier/reset samples, models\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
