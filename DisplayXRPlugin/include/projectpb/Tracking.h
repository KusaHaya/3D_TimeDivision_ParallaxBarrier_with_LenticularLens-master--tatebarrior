#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include <string>

namespace projectpb {
using Point = std::array<float, 3>;
struct CameraTransform {
    float tilt = .13946f; // provisional legacy mount angle, radians
    Point translation{0.f, -.18f, .11f}; // display-space metres
    Point Apply(const Point& camera) const;
};
struct EyeSample {
    Point left{}, right{};
    int64_t receivedNs = 0; // RECEIPT time; current OSC contains no capture time
    uint64_t sequence = 0;
    bool valid = false;
    bool fresh = false; // transport freshness, never a claim of physical tracker lock
};
class EyePairBuffer {
public:
    explicit EyePairBuffer(CameraTransform transform = {}) : transform_(transform) {}
    bool Accept(const std::string& address, const Point& camera, int64_t receivedNs);
    EyeSample Read(int64_t targetNs) const;
private:
    CameraTransform transform_;
    mutable std::mutex mutex_;
    std::array<Point, 2> pending_{};
    std::array<int64_t, 2> pendingTime_{};
    std::array<bool, 2> dirty_{};
    std::array<EyeSample, 32> history_{};
    uint64_t sequence_ = 0;
};
} // namespace projectpb
