#include "RealSenseTracker.h"
#include "RealSenseTrackingMath.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <atomic>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

#include "TemporalEyeDetection.h"

struct RealSenseTracker::Impl {
    std::atomic<bool> stop{false};
    std::thread worker;
    std::mutex mutex;
    tracking::LatestSample latest;
    cv::CascadeClassifier faceDetector, eyeDetector;

    void Invalidate() {
        std::lock_guard<std::mutex> lock(mutex);
        latest.Invalidate();
    }
    tracking::LatestSample::Clock::time_point lastDiagnostic{};
    void ReportDetectionGap(const char* reason) {
        const auto now = tracking::LatestSample::Clock::now();
        if (now - lastDiagnostic < std::chrono::seconds(2)) return;
        bool holding;
        {
            std::lock_guard<std::mutex> lock(mutex);
            std::array<float, 6> ignored;
            holding = latest.Read(ignored, now);
        }
        std::cerr << "RealSense detection: " << reason
                  << (holding ? "; holding last valid position; not a new measurement.\n"
                              : "; no recent valid sample.\n");
        lastDiagnostic = now;
    }
    void Run() {
        tracking::TemporalEyeDetector detector;
        while (!stop) {
            try {
                rs2::pipeline pipeline;
                rs2::config config;
                config.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);
                config.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
                const auto profile = pipeline.start(config);
                const float scale = profile.get_device().first<rs2::depth_sensor>().get_depth_scale();
                rs2::align align(RS2_STREAM_COLOR);
                std::cout << "RealSense D435 tracking started. Face camera, then press h.\n";
                auto lastFrame = tracking::LatestSample::Clock::now();
                while (!stop) {
                    rs2::frameset frames;
                    if (!pipeline.try_wait_for_frames(&frames, 100)) {
                        ReportDetectionGap("frame timeout");
                        if (tracking::LatestSample::Clock::now() - lastFrame > std::chrono::seconds(2))
                            throw std::runtime_error("No RealSense frames for two seconds");
                        continue;
                    }
                    lastFrame = tracking::LatestSample::Clock::now();
                    frames = align.process(frames).as<rs2::frameset>();
                    const auto color = frames.get_color_frame();
                    const auto depth = frames.get_depth_frame();
                    if (!color || !depth) { ReportDetectionGap("color or depth frame missing"); continue; }
                    cv::Mat bgr(color.get_height(), color.get_width(), CV_8UC3,
                        const_cast<void*>(color.get_data()), color.get_stride_in_bytes());
                    cv::Mat depths(depth.get_height(), depth.get_width(), CV_16UC1,
                        const_cast<void*>(depth.get_data()), depth.get_stride_in_bytes());
                    std::array<cv::Point, 2> eyes;
                    std::array<float, 6> coordinates;
                    const auto intrinsics = depth.get_profile().as<rs2::video_stream_profile>().get_intrinsics();
                    const char* reason = "valid eye pair not detected";
                    if (!detector.Detect(bgr, faceDetector, eyeDetector, eyes, reason)) {
                        ReportDetectionGap(reason);
                        continue;
                    }
                    if (!tracking::EyeCoordinates(depths, eyes, scale, intrinsics, coordinates, detector.estimatedEye)) {
                        ReportDetectionGap("invalid eye depth or eye separation");
                        continue;
                    }
                    const auto measuredAt = tracking::LatestSample::Clock::now();
                    bool accepted;
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        accepted = latest.TryUpdate(coordinates, measuredAt);
                    }
                    if (accepted) {
                        detector.Commit(eyes);
                        if(detector.estimatedEye>=0 && measuredAt-lastDiagnostic>=std::chrono::seconds(2)) {
                            std::cout << "RealSense: ESTIMATED eye from one eye + face; visible-eye depth used.\n";
                            lastDiagnostic=measuredAt;
                        }
                    }
                    else ReportDetectionGap("discontinuous 3D sample rejected");
                }
                pipeline.stop();
            } catch (const std::exception& e) {
                // Preserve the last position during camera reconnection.
                std::cerr << "RealSense: " << e.what() << "; retrying in 2 seconds.\n";
                for (int i = 0; i < 20 && !stop; ++i) std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        Invalidate();
    }
};

RealSenseTracker::RealSenseTracker() : impl_(new Impl) {}
RealSenseTracker::~RealSenseTracker() { Stop(); }
bool RealSenseTracker::Start(const std::string& directory) {
    Stop();
    try {
        if (!impl_->faceDetector.load(directory + "/haarcascade_frontalface_default.xml") ||
            !impl_->eyeDetector.load(directory + "/haarcascade_eye_tree_eyeglasses.xml")) {
            std::cerr << "RealSense: face models missing in " << directory << '\n';
            return false;
        }
        impl_->stop = false;
        impl_->worker = std::thread(&Impl::Run, impl_.get());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "RealSense tracking startup failed: " << e.what() << '\n';
        return false;
    }
}
void RealSenseTracker::Stop() {
    impl_->stop = true;
    if (impl_->worker.joinable()) impl_->worker.join();
    impl_->Invalidate();
}
bool RealSenseTracker::Poll(std::array<float, 6>& coordinates) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->latest.Read(coordinates, tracking::LatestSample::Clock::now());
}
