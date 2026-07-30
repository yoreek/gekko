#pragma once

#include <cstddef>
#include <cstdint>

#if defined(ARDUINO) && __has_include(<soc/soc_caps.h>) && __has_include(<esp32-hal-rmt.h>)
#include <soc/soc_caps.h>
#if defined(SOC_RMT_CHANNELS_PER_GROUP) && SOC_RMT_CHANNELS_PER_GROUP > 0
#define EWFM_HAS_RMT_PULSE_CAPTURE 1
#include <esp32-hal-rmt.h>
#endif
#endif

#ifndef EWFM_HAS_RMT_PULSE_CAPTURE
#define EWFM_HAS_RMT_PULSE_CAPTURE 0
#endif

namespace ewfm {

struct PulseCaptureSample {
    bool level{false};
    uint16_t durationMicros{0U};
};

enum class PulseCaptureResult : uint8_t {
    Pending,
    Complete,
    Failed,
    Unsupported,
};

// Asynchronous RMT receive backend. It records a bounded sequence of level/duration pairs and
// intentionally contains no sensor-specific framing or decoding.
class RmtPulseCapture final {
public:
    RmtPulseCapture() = default;
    RmtPulseCapture(const RmtPulseCapture&) = delete;
    RmtPulseCapture& operator=(const RmtPulseCapture&) = delete;
    ~RmtPulseCapture();

    static constexpr bool supported() {
        return EWFM_HAS_RMT_PULSE_CAPTURE != 0;
    }

    bool prepare(uint8_t pin);
    bool start(uint8_t pin);
    PulseCaptureResult poll(PulseCaptureSample* samples, size_t maxSamples, size_t& sampleCount);
    void cancel();
    void release();

private:
#if EWFM_HAS_RMT_PULSE_CAPTURE
    static constexpr size_t kRmtItems = 64U;
    rmt_obj_t* handle_{nullptr};
    uint8_t pin_{0xFFU};
    rmt_data_t items_[kRmtItems]{};
#endif
    bool pending_{false};
};

} // namespace ewfm
