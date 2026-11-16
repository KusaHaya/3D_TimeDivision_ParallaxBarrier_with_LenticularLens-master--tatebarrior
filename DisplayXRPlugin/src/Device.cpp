#include "PluginInternal.h"
#include <cmath>
#include <cstring>
#include <new>

namespace projectpb {
namespace {
struct Device {
    xrt_device base{}; // ABI prefix
    xrt_hmd_parts hmd{};
    xrt_tracking_origin origin{};
    xrt_input input{};
    Settings settings;
    std::shared_ptr<OscReceiver> receiver;
};
xrt_result_t Pose(xrt_device*, xrt_input_name name, int64_t, xrt_space_relation* out) {
    if (!out || name != XRT_INPUT_GENERIC_HEAD_POSE) return XRT_ERROR_DEVICE_CREATION_FAILED;
    *out = {}; out->pose.orientation.w = 1; out->pose.position.z = -.65f;
    out->relation_flags = static_cast<xrt_space_relation_flags>(
        XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_POSITION_VALID_BIT);
    return XRT_SUCCESS;
}
xrt_result_t Views(xrt_device* dev, const xrt_vec3* ipd, int64_t when, uint32_t count,
                   xrt_space_relation* head, xrt_fov* fov, xrt_pose* pose) {
    if (!head || !fov || !pose || count < 1 || count > 2) return XRT_ERROR_DEVICE_CREATION_FAILED;
    Pose(dev, XRT_INPUT_GENERIC_HEAD_POSE, when, head);
    for (uint32_t n=0; n<count; ++n) {
        pose[n] = {}; pose[n].orientation.w = 1;
        if (count == 2) pose[n].position.x = (n ? .5f : -.5f) * (ipd ? ipd->x : .063f);
        fov[n] = dev->hmd->distortion.fov[n];
    }
    return XRT_SUCCESS;
}
}
xrt_device* CreateDevice(const Settings& settings, const Panel&) {
    auto d = std::make_unique<Device>();
    d->settings = settings; d->receiver = AcquireTracking(settings);
    if (settings.tracking && !d->receiver) return nullptr;
    auto& b = d->base;
    b.name = XRT_DEVICE_GENERIC_HMD; b.device_type = XRT_DEVICE_TYPE_HMD;
    strcpy_s(b.str, "ProjectPB 144Hz fixed-phase diagnostic"); strcpy_s(b.serial, "projectpb-hold-0");
    b.hmd = &d->hmd; b.tracking_origin = &d->origin;
    d->origin.type = XRT_TRACKING_TYPE_OTHER; d->origin.initial_offset.orientation.w = 1;
    strcpy_s(d->origin.name, "ProjectPB display");
    b.input_count = 1; b.inputs = &d->input; d->input.active = true; d->input.name = XRT_INPUT_GENERIC_HEAD_POSE;
    b.update_inputs = [](xrt_device*) { return XRT_SUCCESS; };
    b.get_tracked_pose = Pose; b.get_view_poses = Views;
    b.destroy = [](xrt_device* dev) { delete reinterpret_cast<Device*>(dev); };
    b.set_property = [](xrt_device* dev, xrt_device_property_id prop, int32_t value) {
        if (prop != XRT_DEVICE_PROPERTY_OUTPUT_MODE || value < 0 || value > 1) return XRT_ERROR_DEVICE_CREATION_FAILED;
        dev->hmd->active_rendering_mode_index = static_cast<uint32_t>(value); return XRT_SUCCESS;
    };
    b.get_property = [](xrt_device* dev, xrt_device_property_id prop, int32_t* value) {
        if (!value || prop != XRT_DEVICE_PROPERTY_OUTPUT_MODE) return XRT_ERROR_DEVICE_CREATION_FAILED;
        *value = static_cast<int32_t>(dev->hmd->active_rendering_mode_index); return XRT_SUCCESS;
    };
    b.rendering_mode_count = 2;
    for (unsigned n=0; n<2; ++n) {
        auto& mode = b.rendering_modes[n]; mode.mode_index = n;
        strcpy_s(mode.mode_name, n ? "ProjectPB stereo HOLD" : "2D");
        mode.view_count = n ? 2 : 1; mode.tile_columns = mode.view_count; mode.tile_rows = 1;
        mode.view_scale_x = 1; mode.view_scale_y = 1; mode.hardware_display_3d = n != 0;
        mode.mode_flags = n && settings.tracking ? XRT_RENDERING_MODE_FLAG_HAS_TRACKING : 0;
    }
    d->hmd.view_count = 2; d->hmd.active_rendering_mode_index = 1;
    d->hmd.screens[0] = {1920, 1080, 6944444};
    d->hmd.blend_modes[0] = XRT_BLEND_MODE_OPAQUE; d->hmd.blend_mode_count = 1;
    d->hmd.distortion.models = XRT_DISTORTION_MODEL_NONE;
    d->hmd.distortion.preferred = XRT_DISTORTION_MODEL_NONE;
    for (unsigned n=0; n<2; ++n) {
        auto& view = d->hmd.views[n];
        view.viewport = {0,0,1920,1080}; view.display = {1920,1080};
        view.rot.v[0] = 1; view.rot.v[3] = 1;
        d->hmd.view_eye_offsets[n] = {n ? .0315f : -.0315f, 0, .65f};
        d->hmd.distortion.fov[n] = {-std::atan(.264f/.65f), std::atan(.264f/.65f),
                                   std::atan(.1485f/.65f), -std::atan(.1485f/.65f)};
    }
    return &d.release()->base;
}
} // namespace projectpb
