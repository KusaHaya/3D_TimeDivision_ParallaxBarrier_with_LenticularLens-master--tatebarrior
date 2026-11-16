#pragma once
#include "xrt/xrt_plugin.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_tracking.h"
#include "xrt/xrt_display_metrics.h"
#include "projectpb/Barrier.h"
#include "OscReceiver.h"
#include <memory>
#include <string>

namespace projectpb {
struct Settings {
    bool valid = true, enabled = false, tracking = false, calibrated = false;
    bool opticalTracking = false;
    float refX = 0, refZ = .65f, horizontalGain = .7f;
    unsigned phase = 0;
    uint16_t port = 8000;
    std::string panel;
    Barrier barrier;
    CameraTransform transform;
};
struct Panel { int left=0, top=0; bool found=false; };
Settings ReadSettings();
Panel FindPanel(const Settings& settings);
std::shared_ptr<OscReceiver> AcquireTracking(const Settings& settings);
xrt_device* CreateDevice(const Settings& settings, const Panel& panel);
xrt_result_t CreateProcessor(void* device, void* context, void* window, xrt_display_processor_d3d11** out);
bool FillEyes(const Settings& settings, const std::shared_ptr<OscReceiver>& receiver,
              int64_t target, xrt_eye_positions* out);
void FillDisplayInfo(const Settings& settings, const Panel& panel, xrt_plugin_display_info* out);
} // namespace projectpb
