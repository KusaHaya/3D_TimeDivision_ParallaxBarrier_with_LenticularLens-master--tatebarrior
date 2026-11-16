#pragma once
#include "projectpb/Tracking.h"
#include <atomic>
#include <memory>
#include <thread>

namespace projectpb {
int64_t MonotonicNs();
class OscReceiver {
public:
    OscReceiver(uint16_t port, CameraTransform transform);
    ~OscReceiver();
    OscReceiver(const OscReceiver&) = delete;
    OscReceiver& operator=(const OscReceiver&) = delete;
    bool Ready() const { return ready_.load(); }
    EyeSample Read(int64_t target) const { return buffer_.Read(target); }
private:
    void Run();
    EyePairBuffer buffer_;
    uintptr_t socket_ = ~uintptr_t(0);
    bool winsock_ = false;
    std::atomic<bool> stop_{false}, ready_{false};
    std::thread worker_;
};
} // namespace projectpb
