#include "RealSenseTracker.h"
#include "RealSenseTrackingMath.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <atomic>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace {
bool DetectEyes(const cv::Mat& color, cv::CascadeClassifier& faceDetector,
    cv::CascadeClassifier& eyeDetector, std::array<cv::Point, 2>& eyes) {
    cv::Mat gray;
    cv::cvtColor(color, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);
    std::vector<cv::Rect> faces, detections;
    faceDetector.detectMultiScale(gray, faces, 1.1, 5, 0, cv::Size(100, 100));
    if (faces.size() != 1) return false;
    const auto face = faces.front();
    const cv::Rect upper(face.x, face.y, face.width, std::max(1, static_cast<int>(face.height * .62)));
    eyeDetector.detectMultiScale(gray(upper), detections, 1.08, 6, 0,
        cv::Size(std::max(18, face.width / 10), std::max(12, face.height / 12)));
    double bestScore = -1;
    for (size_t i = 0; i < detections.size(); ++i)
        for (size_t j = i + 1; j < detections.size(); ++j) {
            cv::Point left = detections[i].tl() + cv::Point(detections[i].width / 2, detections[i].height / 2) + upper.tl();
            cv::Point right = detections[j].tl() + cv::Point(detections[j].width / 2, detections[j].height / 2) + upper.tl();
            if (left.x > right.x) std::swap(left, right);
            const int separation = right.x - left.x, vertical = std::abs(right.y - left.y);
            if (separation < .2 * face.width || separation > .75 * face.width || vertical > .2 * face.height) continue;
            const double score = detections[i].area() + detections[j].area() + 10. * separation - 15. * vertical;
            if (score > bestScore) { bestScore = score; eyes = {{left, right}}; }
        }
    return bestScore >= 0;
}
}

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
    void Run() {
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
                        Invalidate();
                        if (tracking::LatestSample::Clock::now() - lastFrame > std::chrono::seconds(2))
                            throw std::runtime_error("No RealSense frames for two seconds");
                        continue;
                    }
                    lastFrame = tracking::LatestSample::Clock::now();
                    frames = align.process(frames).as<rs2::frameset>();
                    const auto color = frames.get_color_frame();
                    const auto depth = frames.get_depth_frame();
                    if (!color || !depth) { Invalidate(); continue; }
                    cv::Mat bgr(color.get_height(), color.get_width(), CV_8UC3,
                        const_cast<void*>(color.get_data()), color.get_stride_in_bytes());
                    cv::Mat depths(depth.get_height(), depth.get_width(), CV_16UC1,
                        const_cast<void*>(depth.get_data()), depth.get_stride_in_bytes());
                    std::array<cv::Point, 2> eyes;
                    std::array<float, 6> coordinates;
                    const auto intrinsics = depth.get_profile().as<rs2::video_stream_profile>().get_intrinsics();
                    if (!DetectEyes(bgr, faceDetector, eyeDetector, eyes) ||
                        !tracking::EyeCoordinates(depths, eyes, scale, intrinsics, coordinates)) {
                        Invalidate();
                        continue;
                    }
                    std::lock_guard<std::mutex> lock(mutex);
                    latest.Update(coordinates, lastFrame);
                }
                pipeline.stop();
            } catch (const std::exception& e) {
                Invalidate();
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
