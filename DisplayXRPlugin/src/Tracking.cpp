#include "projectpb/Tracking.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace projectpb {
Point CameraTransform::Apply(const Point& p) const {
    // Raw RealSense +X right, +Y down, +Z forward. Legacy renderer's X-left
    // convention is NOT applied here: DisplayXR requires +X right.
    const float c = std::cos(tilt), s = std::sin(tilt);
    return {p[0] + translation[0], -c*p[1] + s*p[2] + translation[1],
            c*p[2] + s*p[1] + translation[2]};
}
bool EyePairBuffer::Accept(const std::string& address, const Point& camera, int64_t now) {
    const int eye = address == "/eye/left" ? 0 : address == "/eye/right" ? 1 : -1;
    if (eye < 0 || now <= 0 || camera[2] < .1f || camera[2] > 10.f) return false;
    for (float v : camera) if (!std::isfinite(v) || std::abs(v) > 10.f) return false;
    const Point point = transform_.Apply(camera);
    for (float v : point) if (!std::isfinite(v)) return false;
    if (point[2] <= .05f) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    pending_[eye] = point; pendingTime_[eye] = now; dirty_[eye] = true;
    if (!dirty_[0] || !dirty_[1]) return false;
    if (std::llabs(pendingTime_[0] - pendingTime_[1]) > 100000000) {
        dirty_[pendingTime_[0] < pendingTime_[1] ? 0 : 1] = false;
        return false;
    }
    EyeSample sample;
    sample.left = pending_[0]; sample.right = pending_[1];
    sample.receivedNs = std::max(pendingTime_[0], pendingTime_[1]);
    sample.sequence = ++sequence_; sample.valid = true;
    history_[(sequence_ - 1) % history_.size()] = sample;
    dirty_ = {};
    return true;
}
EyeSample EyePairBuffer::Read(int64_t target) const {
    std::lock_guard<std::mutex> lock(mutex_);
    EyeSample result;
    // Zero-order hold of a RECEIVED sample at/before the query, not prediction.
    for (const auto& s : history_) {
        if (s.valid && s.receivedNs <= target && s.sequence > result.sequence) result = s;
    }
    result.fresh = result.valid && target - result.receivedNs <= 250000000;
    return result;
}
} // namespace projectpb
