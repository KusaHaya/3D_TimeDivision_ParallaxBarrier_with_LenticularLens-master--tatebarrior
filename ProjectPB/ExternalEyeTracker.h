#pragma once
#include <array>
#include <memory>

// 外部EyeTracker-RSからOSCを受信する。表示側ではカメラを開かない。
class ExternalEyeTracker {
public:
    ExternalEyeTracker();
    ~ExternalEyeTracker();
    bool Start();
    void Stop();
    bool Poll(std::array<float, 6>& coordinates);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
