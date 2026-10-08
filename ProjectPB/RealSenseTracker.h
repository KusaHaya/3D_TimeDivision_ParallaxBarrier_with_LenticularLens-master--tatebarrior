#pragma once
#include <array>
#include <memory>
#include <string>

// Camera acquisition and face detection run on a worker thread. Poll never
// waits for a frame; the renderer owns all display/calibration state.
class RealSenseTracker {
public:
    RealSenseTracker();
    ~RealSenseTracker();
    RealSenseTracker(const RealSenseTracker&) = delete;
    RealSenseTracker& operator=(const RealSenseTracker&) = delete;
    bool Start(const std::string& modelDirectory);
    void Stop();
    bool Poll(std::array<float, 6>& coordinates);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
