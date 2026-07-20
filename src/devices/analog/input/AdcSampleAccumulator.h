#pragma once

#include <cstdint>

namespace ewfm {

// Accumulates raw millivolt samples for averaging. Used both by a single-tick synchronous ADC
// poll (AnalogPortInputDevice, which can read the pin as many times as it likes within one call)
// and by a multi-tick hub-channel poll (AnalogInputHubChannelDeviceBase), where each sample costs
// a full RequestChannel/.../Ready round trip through the hub's arbitration.
class AdcSampleAccumulator {
public:
    void reset(uint8_t targetSampleCount) {
        target_ = targetSampleCount == 0U ? 1U : targetSampleCount;
        count_ = 0U;
        sum_ = 0;
    }

    void add(int32_t milliVolts) {
        sum_ += milliVolts;
        ++count_;
    }

    bool complete() const {
        return count_ >= target_;
    }

    uint8_t count() const {
        return count_;
    }

    int32_t average() const {
        return count_ == 0U ? 0 : static_cast<int32_t>(sum_ / static_cast<int64_t>(count_));
    }

private:
    uint8_t target_{1};
    uint8_t count_{0};
    int64_t sum_{0};
};

} // namespace ewfm
