#include "devices/pulse/RmtPulseCapture.h"

#include <cstring>

namespace ewfm {

RmtPulseCapture::~RmtPulseCapture() {
    release();
}

bool RmtPulseCapture::prepare(uint8_t pin) {
#if EWFM_HAS_RMT_PULSE_CAPTURE
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    // Core 3.x's RMT driver is pin-addressed (no channel handle): rmtInit() takes the
    // tick frequency directly (1000 ns tick == 1,000,000 Hz) and rmtSetRxMaxThreshold()
    // replaces the old handle-based rmtSetRxThreshold().
    if (pin_ != pin) {
        cancel();
        if (pin_ != 0xFFU) {
            (void)rmtDeinit(pin_);
        }
        if (!rmtInit(pin, RMT_RX_MODE, RMT_MEM_NUM_BLOCKS_1, 1000000U)) {
            pin_ = 0xFFU;
            return false;
        }
        pin_ = pin;
        (void)rmtSetRxMaxThreshold(pin, 150U);
    }
    return true;
#else
    if (handle_ == nullptr || pin_ != pin) {
        cancel();
        if (handle_ != nullptr) {
            (void)rmtDeinit(handle_);
        }
        handle_ = rmtInit(pin, RMT_RX_MODE, RMT_MEM_64);
        pin_ = pin;
        if (handle_ == nullptr) {
            return false;
        }
        (void)rmtSetTick(handle_, 1000.0F);
        (void)rmtSetRxThreshold(handle_, 150U);
    }
    return true;
#endif
#else
    (void)pin;
    return false;
#endif
}

bool RmtPulseCapture::start(uint8_t pin) {
#if EWFM_HAS_RMT_PULSE_CAPTURE
    cancel();
    if (!prepare(pin)) {
        return false;
    }
    std::memset(items_, 0, sizeof(items_));
    size_t requested = kRmtItems;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    pending_ = rmtReadAsync(pin, items_, &requested);
#else
    pending_ = rmtReadAsync(handle_, items_, kRmtItems, nullptr, false, 0U);
#endif
    return pending_;
#else
    (void)pin;
    return false;
#endif
}

PulseCaptureResult RmtPulseCapture::poll(PulseCaptureSample* samples, size_t maxSamples, size_t& sampleCount) {
    sampleCount = 0U;
#if EWFM_HAS_RMT_PULSE_CAPTURE
    if (!pending_) {
        return PulseCaptureResult::Failed;
    }
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    if (!rmtReceiveCompleted(pin_)) {
        return PulseCaptureResult::Pending;
    }
#else
    if (!rmtReceiveCompleted(handle_)) {
        return PulseCaptureResult::Pending;
    }
#endif
    pending_ = false;
    for (const rmt_data_t& item : items_) {
        const uint16_t durations[] = {static_cast<uint16_t>(item.duration0), static_cast<uint16_t>(item.duration1)};
        const bool levels[] = {item.level0 != 0U, item.level1 != 0U};
        for (size_t index = 0U; index < 2U; ++index) {
            if (durations[index] == 0U) {
                return sampleCount == 0U ? PulseCaptureResult::Failed : PulseCaptureResult::Complete;
            }
            if (sampleCount >= maxSamples) {
                return PulseCaptureResult::Complete;
            }
            samples[sampleCount++] = {levels[index], durations[index]};
        }
    }
    return sampleCount == 0U ? PulseCaptureResult::Failed : PulseCaptureResult::Complete;
#else
    (void)samples;
    (void)maxSamples;
    return PulseCaptureResult::Unsupported;
#endif
}

void RmtPulseCapture::cancel() {
#if EWFM_HAS_RMT_PULSE_CAPTURE
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    // Core 3.x has no explicit "abort a pending async read" call; deiniting the pin's
    // channel is the only way to stop it, so only do that in release(), not here -- a
    // cancelled-but-still-prepared channel is reused by the next start() on the same pin.
#else
    if (pending_ && handle_ != nullptr) {
        (void)rmtEnd(handle_);
    }
#endif
#endif
    pending_ = false;
}

void RmtPulseCapture::release() {
    cancel();
#if EWFM_HAS_RMT_PULSE_CAPTURE
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    if (pin_ != 0xFFU) {
        (void)rmtDeinit(pin_);
        pin_ = 0xFFU;
    }
#else
    if (handle_ != nullptr) {
        (void)rmtDeinit(handle_);
        handle_ = nullptr;
        pin_ = 0xFFU;
    }
#endif
#endif
}

} // namespace ewfm
