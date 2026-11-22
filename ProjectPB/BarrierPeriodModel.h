#pragma once
#include <cmath>

// Optional model only: not connected to the production tracking/render loop.
// All lengths are mm. haba is the insertion interval in subpixel coordinates.
namespace barrier_period_model {
inline bool positive(double x) { return std::isfinite(x) && x > 0.0; }
inline bool supportedRate(double rate, double& haba) {
    if (!positive(rate) || rate < 1.0 / 100000.0 - 1e-14 || rate > 0.5 + 1e-14) return false;
    rate = rate < 1.0 / 100000.0 ? 1.0 / 100000.0 : (rate > 0.5 ? 0.5 : rate);
    const double candidate = 1.0 / rate;
    if (!std::isfinite(candidate)) return false;
    haba = candidate;
    return true;
}

// Use measured calibration at (referenceDistanceMm, referenceHaba).
// A failed result must be handled explicitly; never send negative/zero haba to the shader.
inline bool fromCalibration(double distanceMm, double referenceDistanceMm,
                            double referenceHaba, double effectiveGapMm,
                            double& haba) {
    if (!positive(distanceMm) || !positive(referenceDistanceMm) ||
        !std::isfinite(referenceHaba) || referenceHaba < 2.0 || referenceHaba > 100000.0 ||
        !std::isfinite(effectiveGapMm) || effectiveGapMm < 0.0) return false;
    const double rate = 1.0 - (1.0 - 1.0 / referenceHaba) *
        (1.0 + effectiveGapMm / distanceMm) /
        (1.0 + effectiveGapMm / referenceDistanceMm);
    return supportedRate(rate, haba);
}

// Absolute model; use actual measured screen period and actual horizontal pixel pitch.
inline bool fromDimensions(double distanceMm, double effectiveGapMm,
                           double screenPeriodMm, double pixelPitchMm,
                           double& haba) {
    if (!positive(distanceMm) || !positive(screenPeriodMm) || !positive(pixelPitchMm) ||
        !std::isfinite(effectiveGapMm) || effectiveGapMm < 0.0) return false;
    const double target = screenPeriodMm * distanceMm / (distanceMm + effectiveGapMm);
    const double base = 8.0 * pixelPitchMm / 3.0;
    return supportedRate(1.0 - base / target, haba);
}

// Tangent to the calibrated curve, for comparison with the existing linear formula.
inline bool linearApproximation(double distanceMm, double referenceDistanceMm,
                                double referenceHaba, double effectiveGapMm,
                                double& haba) {
    double reference;
    if (!fromCalibration(referenceDistanceMm, referenceDistanceMm, referenceHaba,
                         effectiveGapMm, reference) || !positive(distanceMm)) return false;
    const double slope = -referenceHaba * (referenceHaba - 1.0) * effectiveGapMm /
        (referenceDistanceMm * (referenceDistanceMm + effectiveGapMm));
    const double candidate = referenceHaba + slope * (distanceMm - referenceDistanceMm);
    if (!std::isfinite(candidate) || candidate < 2.0 || candidate > 100000.0) return false;
    haba = candidate;
    return true;
}
}
