#pragma once

#include "devices/sensors/filter/SensorFilterConfig.h"

namespace ewfm {

// Fixed two-stage reading filter: linear calibration (scale+offset) applied first, then
// exponential smoothing. Stack-resident, no heap allocation, no virtual dispatch.
class SensorReadingFilter {
public:
    void configure(const SensorFilterConfigV1& config) {
        config_ = config;
    }

    // Clears the seeded state so the next apply() reseeds instead of blending with a stale value.
    void reset() {
        seeded_ = false;
        smoothedValue_ = 0.0F;
    }

    float apply(float rawValue) {
        const float calibrated = rawValue * config_.calibrationFactor + config_.calibrationOffset;
        if (!seeded_) {
            smoothedValue_ = calibrated;
            seeded_ = true;
        } else {
            smoothedValue_ = config_.smoothingWeight * calibrated + (1.0F - config_.smoothingWeight) * smoothedValue_;
        }
        return smoothedValue_;
    }

    float currentValue() const {
        return smoothedValue_;
    }

private:
    SensorFilterConfigV1 config_{};
    float smoothedValue_{0.0F};
    bool seeded_{false};
};

} // namespace ewfm
