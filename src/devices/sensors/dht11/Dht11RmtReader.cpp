#include "devices/sensors/dht11/Dht11RmtReader.h"

#include "devices/sensors/dht11/Dht11Protocol.h"

namespace ewfm {

Dht11RmtReader::Result Dht11RmtReader::poll(IDht11LineDriver& lineDriver, uint8_t pin, bool internalPullup, uint32_t now,
                                            int32_t& milliCelsius, int32_t& milliPercent, const char*& errorStatus) {
    if (!RmtPulseCapture::supported()) {
        errorStatus = "rmt_unsupported";
        return Result::Unsupported;
    }
    if (phase_ == Phase::Idle) {
        if (!lineDriver.driveLow(pin)) {
            errorStatus = "not_found";
            return Result::Failed;
        }
        startLowAt_ = now;
        phase_ = Phase::StartLow;
        return Result::Pending;
    }
    if (phase_ == Phase::StartLow) {
        if (static_cast<uint32_t>(now - startLowAt_) < 18U) {
            return Result::Pending;
        }
        if (!lineDriver.release(pin, internalPullup) || !capture_.start(pin)) {
            phase_ = Phase::Idle;
            errorStatus = "rmt_unavailable";
            return Result::Failed;
        }
        phase_ = Phase::Capturing;
        return Result::Pending;
    }

    size_t sampleCount = 0U;
    const PulseCaptureResult captureResult = capture_.poll(pulses_, sizeof(pulses_) / sizeof(pulses_[0]), sampleCount);
    if (captureResult == PulseCaptureResult::Pending) {
        return Result::Pending;
    }
    phase_ = Phase::Idle;
    if (captureResult != PulseCaptureResult::Complete || sampleCount < kDht11DataPulseCount + 2U) {
        errorStatus = captureResult == PulseCaptureResult::Unsupported ? "rmt_unsupported" : "timeout";
        return captureResult == PulseCaptureResult::Unsupported ? Result::Unsupported : Result::Failed;
    }
    // The DHT11 response preamble is a low then a high pulse. The following 80 pulses carry 40 bits.
    if (pulses_[0].level || !pulses_[1].level) {
        errorStatus = "protocol_error";
        return Result::Failed;
    }
    return dht11DecodePulsePairs(pulses_ + 2U, sampleCount - 2U, milliCelsius, milliPercent, errorStatus) ? Result::Ready : Result::Failed;
}

void Dht11RmtReader::cancel() {
    capture_.cancel();
    phase_ = Phase::Idle;
}

void Dht11RmtReader::reset() {
    capture_.release();
    phase_ = Phase::Idle;
}

} // namespace ewfm
