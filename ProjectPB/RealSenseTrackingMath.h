#pragma once
#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>
#include <opencv2/core.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <vector>

namespace tracking {
inline bool MedianDepth(const cv::Mat& depth, cv::Point pixel, float scale, float& metres) {
    if (depth.type() != CV_16UC1 || !std::isfinite(scale) || scale <= 0 ||
        pixel.x < 0 || pixel.y < 0 || pixel.x >= depth.cols || pixel.y >= depth.rows) return false;
    const cv::Rect patch = cv::Rect(pixel.x - 4, pixel.y - 4, 9, 9) & cv::Rect(0, 0, depth.cols, depth.rows);
    std::vector<float> valid;
    for (int y = patch.y; y < patch.y + patch.height; ++y)
        for (int x = patch.x; x < patch.x + patch.width; ++x) {
            const float value = depth.at<unsigned short>(y, x) * scale;
            if (std::isfinite(value) && value > 0.1f && value < 4.f) valid.push_back(value);
        }
    if (valid.size() < 9 || valid.size() * 2 < static_cast<size_t>(patch.area())) return false;
    const auto middle = valid.begin() + valid.size() / 2;
    std::nth_element(valid.begin(), middle, valid.end());
    metres = *middle;
    return true;
}
inline bool EyeCoordinates(const cv::Mat& depth, const std::array<cv::Point, 2>& eyes,
    float scale, const rs2_intrinsics& intrinsics, std::array<float, 6>& result, int estimatedEye = -1) {
    if (estimatedEye < -1 || estimatedEye > 1) return false;
    float distances[2]{};
    for(int i=0;i<2;++i) {
        if (i!=estimatedEye && !MedianDepth(depth,eyes[i],scale,distances[i])) return false;
    }
    // Estimated eye uses visible-eye depth: frontal-face planar approximation.
    if(estimatedEye>=0) distances[estimatedEye]=distances[1-estimatedEye];
    for (int i = 0; i < 2; ++i) {
        const float metres=distances[i];
        const float pixel[2] = {static_cast<float>(eyes[i].x), static_cast<float>(eyes[i].y)};
        float point[3];
        rs2_deproject_pixel_to_point(point, &intrinsics, pixel, metres);
        // Preserve renderer axes (X left, Y up, Z forward) and metres.
        result[i * 3] = -point[0];
        result[i * 3 + 1] = -point[1];
        result[i * 3 + 2] = point[2];
    }
    for (float value : result) if (!std::isfinite(value)) return false;
    float separationSquared = 0;
    for (int i = 0; i < 3; ++i) {
        const float delta = result[i] - result[i + 3];
        separationSquared += delta * delta;
    }
    return separationSquared >= 0.035f * 0.035f && separationSquared <= 0.09f * 0.09f;
}

// Accessed only under the tracker's mutex. Kept separately for camera-free tests.
struct LatestSample {
    using Clock = std::chrono::steady_clock;
    std::array<float, 6> coordinates{};
    Clock::time_point received{};
    bool valid = false;
    std::array<float, 6> lastRaw{};
    std::array<float, 6> pending{};
    Clock::time_point pendingAt{};
    int pendingCount = 0;
    bool TryUpdate(const std::array<float, 6>& value, Clock::time_point now) {
        for (float v : value) if (!std::isfinite(v)) return false;
        bool jump = false;
        if (valid) {
            const float seconds = std::clamp(std::chrono::duration<float>(now-received).count(), 0.f, .2f);
            const float limit = .04f + .6f * seconds;
            for (int eye=0; eye<2; ++eye) {
                float distance=0;
                for(int axis=0; axis<3; ++axis) { float d=value[eye*3+axis]-lastRaw[eye*3+axis]; distance+=d*d; }
                if(distance > limit*limit) jump = true;
            }
        }
        if (jump) {
            bool consistent = pendingCount > 0 && now-pendingAt < std::chrono::milliseconds(300);
            for (int eye=0; eye<2; ++eye) {
                float distance = 0;
                for (int axis=0; axis<3; ++axis) {
                    const float d=value[eye*3+axis]-pending[eye*3+axis]; distance+=d*d;
                }
                consistent = consistent && distance < .035f*.035f;
            }
            pendingCount = consistent ? pendingCount+1 : 1;
            pending = value; pendingAt = now;
            // Keep the held output until three consecutive consistent candidates.
            if (pendingCount < 3) return false;
        }
        pendingCount = 0;
        Update(value, now); return true;
    }
    void Invalidate() { valid = false; pendingCount = 0; }
    void Update(const std::array<float, 6>& value, Clock::time_point now) {
        const float dt = std::chrono::duration<float>(now - received).count();
        const float alpha = valid ? 1.f - std::exp(-std::max(0.f, dt) / 0.1f) : 1.f;
        for (size_t i = 0; i < value.size(); ++i)
            coordinates[i] = valid ? coordinates[i] + alpha * (value[i] - coordinates[i]) : value[i];
        lastRaw = value;
        received = now;
        valid = true;
    }
    bool Read(std::array<float, 6>& value, Clock::time_point now) const {
        // A detection gap holds the last measurement; only explicit invalidation clears it.
        if (!valid) return false;
        value = coordinates;
        return true;
    }
};
}
