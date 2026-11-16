#include "PluginInternal.h"
#include <windows.h>
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>

namespace projectpb {
namespace {
std::atomic<xrt_component_get_setting_fn_t> getSetting{nullptr};
const xrt_component_setting_desc schema[] = {
    {"ENABLE", "bool", "0", "Opt in to experimental fixed-phase output (no LED control)"},
    {"PANEL_DEVICE_NAME", "string", "", "Exact Windows display name, e.g. \\\\.\\DISPLAY2; must be 1920x1080 at 144Hz"},
    {"PHASE", "int", "0", "Fixed optical phase 0..3; manually match the legacy LED HOLD phase"},
    {"TRACKING", "bool", "0", "Receive raw EyeTracker-RS OSC; requires CALIBRATION_CONFIRMED=1"},
    {"CALIBRATION_CONFIRMED", "bool", "0", "Camera-to-display extrinsics have been measured; no inferred tracker-lock claim"},
    {"OSC_PORT", "int", "8000", "Exclusive IPv4 loopback UDP input; do not run the legacy receiver concurrently"},
    {"CAM_TILT_RAD", "float", "0.13946", "Provisional legacy tilt, radians; measure before tracking"},
    {"CAM_X_M", "float", "0", "Display +X-right camera translation, metres"},
    {"CAM_Y_M", "float", "-0.18", "Display +Y-up camera translation, metres"},
    {"CAM_Z_M", "float", "0.11", "Display +Z-toward-viewer camera translation, metres"},
    {"HABA", "float", "330", "Barrier correction width in subpixels"},
    {"MIDDLE_LINE", "float", "3420", "Optical calibration reference in subpixels"},
    {"SHIFT", "float", "1", "Manual phase offset in subpixels"},
    {"SLANT_Y", "float", "1", "Barrier slope per GL bottom-up panel row"},
    {"CORRECTION_THRESHOLD", "float", "0.8", "Symmetric quantisation threshold, 0.05..1"},
    {"OPTICAL_TRACKING", "bool", "0", "Apply legacy barrier tracking from the same OSC eyes; requires calibrated TRACKING"},
    {"REFERENCE_X_M", "float", "0", "Measured display +X-right head centre at manual optical alignment"},
    {"REFERENCE_Z_M", "float", "0.65", "Measured head centre depth at manual optical alignment, metres"},
    {"HORIZONTAL_GAIN", "float", "0.7", "Legacy optical correction gain; never scales OpenXR eye positions"},
};
std::string Value(const char* key, const char* fallback) {
    char value[512]{};
    const auto fn = getSetting.load();
    if (fn && fn("projectpb", key, value, sizeof(value))) {
        value[sizeof(value)-1] = 0;
        return value;
    }
    return fallback;
}
double Number(const char* key, const char* fallback, double low, double high, bool integral, bool& valid) {
    const std::string text = Value(key, fallback);
    char* end = nullptr; errno = 0;
    const double n = std::strtod(text.c_str(), &end);
    if (text.empty() || !end || *end || errno || !std::isfinite(n) || n < low || n > high ||
        (integral && n != std::floor(n))) { valid = false; return std::strtod(fallback, nullptr); }
    return n;
}
template<class T> void CopyOut(T* out, const T& value) {
    const uint32_t size = out->struct_size;
    std::memcpy(out, &value, std::min<size_t>(size, sizeof(T)));
    out->struct_size = size;
}
}
Settings ReadSettings() {
    Settings s;
    s.enabled = Number("ENABLE", "0", 0, 1, true, s.valid) != 0;
    s.phase = static_cast<unsigned>(Number("PHASE", "0", 0, 3, true, s.valid));
    s.tracking = Number("TRACKING", "0", 0, 1, true, s.valid) != 0;
    s.calibrated = Number("CALIBRATION_CONFIRMED", "0", 0, 1, true, s.valid) != 0;
    s.port = static_cast<uint16_t>(Number("OSC_PORT", "8000", 1, 65535, true, s.valid));
    s.panel = Value("PANEL_DEVICE_NAME", "");
    s.transform.tilt = static_cast<float>(Number("CAM_TILT_RAD", "0.13946", -3.142, 3.142, false, s.valid));
    for (int a=0; a<3; ++a) {
        const char* names[] = {"CAM_X_M", "CAM_Y_M", "CAM_Z_M"};
        const char* defaults[] = {"0", "-0.18", "0.11"};
        s.transform.translation[a] = static_cast<float>(Number(names[a], defaults[a], -10, 10, false, s.valid));
    }
    s.barrier.haba = static_cast<float>(Number("HABA", "330", 2, 100000, false, s.valid));
    s.barrier.middle = static_cast<float>(Number("MIDDLE_LINE", "3420", -1000000, 1000000, false, s.valid));
    s.barrier.shift = static_cast<float>(Number("SHIFT", "1", -1000, 1000, false, s.valid));
    s.barrier.slant = static_cast<float>(Number("SLANT_Y", "1", -10, 10, false, s.valid));
    s.barrier.threshold = static_cast<float>(Number("CORRECTION_THRESHOLD", "0.8", .05, 1, false, s.valid));
    s.opticalTracking = Number("OPTICAL_TRACKING", "0", 0, 1, true, s.valid) != 0;
    s.refX = static_cast<float>(Number("REFERENCE_X_M", "0", -5, 5, false, s.valid));
    s.refZ = static_cast<float>(Number("REFERENCE_Z_M", "0.65", .05, 10, false, s.valid));
    s.horizontalGain = static_cast<float>(Number("HORIZONTAL_GAIN", "0.7", .1, 3, false, s.valid));
    if (s.tracking && !s.calibrated) s.valid = false;
    if (s.opticalTracking && !s.tracking) s.valid = false;
    return s;
}
Panel FindPanel(const Settings& s) {
    Panel panel;
    if (!s.valid || !s.enabled || s.panel.empty()) return panel;
    DISPLAY_DEVICEA dd{}; dd.cb = sizeof(dd);
    for (DWORD index=0; EnumDisplayDevicesA(nullptr, index, &dd, 0); ++index) {
        if (s.panel != dd.DeviceName || !(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) continue;
        DEVMODEA mode{}; mode.dmSize = sizeof(mode);
        if (!EnumDisplaySettingsA(dd.DeviceName, ENUM_CURRENT_SETTINGS, &mode) ||
            mode.dmPelsWidth != 1920 || mode.dmPelsHeight != 1080 || mode.dmDisplayFrequency != 144 ||
            mode.dmDisplayOrientation != DMDO_DEFAULT) return panel;
        panel.left = mode.dmPosition.x; panel.top = mode.dmPosition.y; panel.found = true;
        return panel;
    }
    return panel;
}
std::shared_ptr<OscReceiver> AcquireTracking(const Settings& s) {
    if (!s.tracking || !s.calibrated) return {};
    static std::mutex mutex;
    static std::weak_ptr<OscReceiver> weak;
    static Settings active;
    std::lock_guard<std::mutex> lock(mutex);
    if (auto receiver = weak.lock()) {
        if (active.port != s.port || active.transform.tilt != s.transform.tilt ||
            active.transform.translation != s.transform.translation) return {};
        return receiver;
    }
    auto receiver = std::make_shared<OscReceiver>(s.port, s.transform);
    if (!receiver->Ready()) return {};
    active = s; weak = receiver;
    return receiver;
}
bool FillEyes(const Settings&, const std::shared_ptr<OscReceiver>& receiver, int64_t target, xrt_eye_positions* out) {
    if (!out) return false;
    *out = {};
    const auto sample = receiver ? receiver->Read(target) : EyeSample{};
    if (sample.valid) {
        out->eyes[0] = {sample.left[0], sample.left[1], sample.left[2]};
        out->eyes[1] = {sample.right[0], sample.right[1], sample.right[2]};
        out->timestamp_ns = sample.receivedNs;
    } else {
        out->eyes[0] = {-.0315f, 0, .65f}; out->eyes[1] = {.0315f, 0, .65f};
        // timestamp 0 explicitly identifies the nominal fallback.
    }
    out->count = 2; out->valid = true;
    // No capture timestamp or physical lock flag exists in current OSC.
    out->is_tracking = false;
    return true;
}
void FillDisplayInfo(const Settings& s, const Panel& p, xrt_plugin_display_info* out) {
    xrt_plugin_display_info info{}; info.struct_size = sizeof(info);
    info.display_width_m = .528f; info.display_height_m = .297f;
    info.nominal_viewer_z_m = .65f;
    info.display_pixel_width = 1920; info.display_pixel_height = 1080;
    info.display_screen_left = p.left; info.display_screen_top = p.top;
    info.refresh_mhz = 144000;
    info.supported_eye_tracking_modes = s.tracking ? 1u : 0u;
    info.display_flags = p.found ? XRT_PLUGIN_DISPLAY_FLAG_SCREEN_ORIGIN_VALID : 0;
    CopyOut(out, info);
}
struct Instance { Settings settings; Panel panel; };
static xrt_result_t Probe(xrt_plugin_instance** out) {
    *out = nullptr;
    try {
        const auto s = ReadSettings(); const auto p = FindPanel(s);
        if (!p.found) return XRT_ERROR_PROBER_NOT_SUPPORTED;
        *out = reinterpret_cast<xrt_plugin_instance*>(new Instance{s,p});
        return XRT_SUCCESS;
    } catch (...) { return XRT_ERROR_DEVICE_CREATION_FAILED; }
}
static xrt_result_t Device(xrt_plugin_instance* inst, xrt_device** out) {
    *out = nullptr;
    if (!inst) return XRT_ERROR_DEVICE_CREATION_FAILED;
    try {
        auto& i = *reinterpret_cast<Instance*>(inst);
        *out = CreateDevice(i.settings, i.panel);
        return *out ? XRT_SUCCESS : XRT_ERROR_DEVICE_CREATION_FAILED;
    } catch (...) { return XRT_ERROR_DEVICE_CREATION_FAILED; }
}
static bool Info(xrt_plugin_instance* inst, xrt_device*, xrt_plugin_display_info* out) {
    if (!inst || !out || out->struct_size < sizeof(uint32_t)) return false;
    const auto& i = *reinterpret_cast<Instance*>(inst);
    FillDisplayInfo(i.settings, i.panel, out); return true;
}
static uint32_t Claims(xrt_plugin_instance* inst, const xrt_display_descriptor* displays, uint32_t count,
                       xrt_display_claim* claims, uint32_t capacity) {
    if (!inst || !displays || !claims || !capacity) return 0;
    const auto& p = reinterpret_cast<Instance*>(inst)->panel;
    for (uint32_t n=0; n<count; ++n) {
        const auto& d = displays[n];
        if (d.struct_size < sizeof(d)) continue;
        if (d.screen_left == p.left && d.screen_top == p.top && d.pixel_width == 1920 &&
            d.pixel_height == 1080 && d.refresh_mhz >= 143500 && d.refresh_mhz <= 144500) {
            claims[0] = {}; claims[0].monitor_id = d.monitor_id;
            claims[0].confidence = XRT_DISPLAY_CLAIM_FALLBACK; // explicit device-name selection, NOT EDID verified
            claims[0].supported_apis = XRT_DP_API_BIT_D3D11;
            return 1;
        }
    }
    return 0;
}
static uint32_t Describe(const xrt_component_setting_desc** out) {
    if (out) *out = schema;
    return static_cast<uint32_t>(sizeof(schema)/sizeof(schema[0]));
}
static bool Platform(xrt_plugin_platform_status* out) {
    if (!out || out->struct_size < sizeof(uint32_t)) return false;
    xrt_plugin_platform_status value{}; value.struct_size = sizeof(value);
    try {
        const auto s = ReadSettings(); const auto p = FindPanel(s);
        value.state = p.found ? XRT_PLUGIN_PLATFORM_STATE_READY : XRT_PLUGIN_PLATFORM_STATE_NO_DISPLAY;
        const char* hint = p.found ? "Fixed-phase diagnostic only; LED control and continuous synchronization are not implemented" :
            "Set ENABLE=1 and exact PANEL_DEVICE_NAME (1920x1080 144Hz); validate camera calibration before TRACKING=1";
        strncpy_s(value.hint, hint, _TRUNCATE);
        CopyOut(out, value); return true;
    } catch (...) { return false; }
}
static xrt_plugin_iface MakeIface() {
    xrt_plugin_iface i{}; i.struct_size = sizeof(i);
    i.id = "projectpb"; i.display_name = "ProjectPB fixed-phase diagnostic";
    i.vendor = "ProjectPB research"; i.version = "0.1.0";
    i.probe = Probe; i.create_device = Device; i.create_dp_d3d11 = CreateProcessor;
    i.destroy = [](xrt_plugin_instance* inst) { delete reinterpret_cast<Instance*>(inst); };
    i.get_display_info = Info; i.probe_displays = Claims;
    i.describe_settings = Describe; i.get_platform_state = Platform;
    return i;
}
} // namespace projectpb

extern "C" XRT_PLUGIN_EXPORT xrt_result_t xrtPluginNegotiate(uint32_t version,
    const xrt_plugin_host_iface* host, xrt_plugin_iface** out, uint32_t* pluginVersion) {
    if (!out || !pluginVersion) return XRT_ERROR_PROBER_NOT_SUPPORTED;
    *out = nullptr; *pluginVersion = XRT_PLUGIN_API_VERSION_CURRENT;
    if (version != XRT_PLUGIN_API_VERSION_CURRENT) return XRT_ERROR_PROBER_NOT_SUPPORTED;
    xrt_component_get_setting_fn_t reader = nullptr;
    if (host && host->struct_size >= offsetof(xrt_plugin_host_iface, get_setting) + sizeof(host->get_setting))
        reader = host->get_setting;
    projectpb::getSetting.store(reader);
    static xrt_plugin_iface iface = projectpb::MakeIface();
    *out = &iface; return XRT_SUCCESS;
}
