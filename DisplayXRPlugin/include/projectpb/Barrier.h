#pragma once
#include <algorithm>
#include <cmath>

namespace projectpb {
struct Barrier {
    float haba = 330;
    float middle = 3420;
    float shift = 1;
    float slant = 1;
    float threshold = .8f;
};
inline float Quantize(float q, float threshold) {
    const float magnitude = std::abs(q);
    const float base = std::floor(magnitude);
    const float value = base + (magnitude >= base + std::clamp(threshold, .05f, 1.f) ? 1.f : 0.f);
    return q < 0 ? -value : value;
}
// Output coordinates are integer panel pixels, TOP-left origin (D3D).
inline bool SelectLeft(int x, int yTop, int height, int channel, unsigned phase, const Barrier& p) {
    const float s = 3.f * x + channel + p.slant * (height - 1 - yTop);
    const float q = (s - (p.middle - 48.f * p.haba)) / std::max(p.haba, 1.f);
    const float v = s - Quantize(q, p.threshold) + 2.f * (phase & 3u) + p.shift;
    const float positive = std::fmod(std::fmod(v, 8.f) + 8.f, 8.f);
    return positive >= 4.f;
}
// Legacy empirical correction. Input is display-centred, +X right, metres.
inline bool TrackedBarrier(float xRight, float z, float refXRight, float refZ,
                           float gain, Barrier& p) {
    if (!std::isfinite(xRight) || !std::isfinite(z) || !std::isfinite(refXRight) ||
        !std::isfinite(refZ) || !std::isfinite(gain) || std::abs(z - .3245f) < .02f) return false;
    p.haba = static_cast<float>(std::clamp(static_cast<int>(p.haba) + static_cast<int>((refZ - z) * 1000.f / 2.f), 2, 100000));
    const float delta = gain * .895f / (z - .3245f) * (refXRight - xRight) * 1000.f / .09f;
    if (!std::isfinite(delta) || std::abs(delta) > 10000000.f) return false;
    p.middle -= std::round(delta);
    return true;
}
} // namespace projectpb
