#include "devices/sensors/dht11/Dht11RmtReader.h"

#include "debug/Debug.h"
#include "devices/sensors/dht11/Dht11Protocol.h"

#include <new>

namespace ewfm {

struct Dht11RmtReader::Session {
    RmtPulseCapture capture{};
    PulseCaptureSample pulses[kDht11DataPulseCount + 2U]{};
};

Dht11RmtReader::Dht11RmtReader() = default;

Dht11RmtReader::~Dht11RmtReader() = default;

Dht11RmtReader::Result Dht11RmtReader::poll(IDht11LineDriver& lineDriver, uint8_t pin, bool internalPullup, uint32_t now,
                                            int32_t& milliCelsius, int32_t& milliPercent, const char*& errorStatus) {
    if (!RmtPulseCapture::supported()) {
        errorStatus = "rmt_unsupported";
        return Result::Unsupported;
    }
    if (!session_) {
        session_.reset(new (std::nothrow) Session());
        if (!session_) {
            errorStatus = "out_of_memory";
            return Result::Failed;
        }
    }
    if (phase_ == Phase::Idle) {
        if (!session_->capture.prepare(pin) || !lineDriver.driveLow(pin)) {
            EWFM_DHT11_LOG_WARN("rmt start failed pin=%u", static_cast<unsigned>(pin));
            errorStatus = "not_found";
            return Result::Failed;
        }
        EWFM_DHT11_LOG_INFO("rmt start pulse pin=%u", static_cast<unsigned>(pin));
        startLowAt_ = now;
        phase_ = Phase::StartLow;
        return Result::Pending;
    }
    if (phase_ == Phase::StartLow) {
        if (static_cast<uint32_t>(now - startLowAt_) < 18U) {
            return Result::Pending;
        }
        if (!session_->capture.start(pin) || !lineDriver.release(pin, internalPullup)) {
            phase_ = Phase::Idle;
            EWFM_DHT11_LOG_WARN("rmt arm/release failed pin=%u", static_cast<unsigned>(pin));
            errorStatus = "rmt_unavailable";
            return Result::Failed;
        }
        EWFM_DHT11_LOG_INFO("rmt capture armed pin=%u", static_cast<unsigned>(pin));
        phase_ = Phase::Capturing;
        return Result::Pending;
    }

    size_t sampleCount = 0U;
    const PulseCaptureResult captureResult =
        session_->capture.poll(session_->pulses, sizeof(session_->pulses) / sizeof(session_->pulses[0]), sampleCount);
    if (captureResult == PulseCaptureResult::Pending) {
        return Result::Pending;
    }
    phase_ = Phase::Idle;
    if (captureResult != PulseCaptureResult::Complete) {
        EWFM_DHT11_LOG_WARN("rmt capture failed result=%u", static_cast<unsigned>(captureResult));
        errorStatus = captureResult == PulseCaptureResult::Unsupported ? "rmt_unsupported" : "timeout";
        return captureResult == PulseCaptureResult::Unsupported ? Result::Unsupported : Result::Failed;
    }
    EWFM_DHT11_LOG_INFO("rmt captured pulses=%u", static_cast<unsigned>(sampleCount));
    for (size_t index = 0U; index < sampleCount; ++index) {
        const PulseCaptureSample& pulse = session_->pulses[index];
        (void)pulse;
        EWFM_DHT11_LOG_INFO("rmt pulse[%u] level=%u duration=%u", static_cast<unsigned>(index), static_cast<unsigned>(pulse.level),
                            static_cast<unsigned>(pulse.durationMicros));
    }
    if (dht11DecodeFramedPulsePairs(session_->pulses, sampleCount, milliCelsius, milliPercent, errorStatus)) {
        EWFM_DHT11_LOG_INFO("rmt frame decoded temperature=%ld humidity=%ld", static_cast<long>(milliCelsius),
                            static_cast<long>(milliPercent));
        return Result::Ready;
    }
    EWFM_DHT11_LOG_WARN("rmt frame decode failed status=%s", errorStatus != nullptr ? errorStatus : "unknown");
    return Result::Failed;
}

void Dht11RmtReader::cancel() {
    if (session_) {
        session_->capture.cancel();
    }
    phase_ = Phase::Idle;
}

void Dht11RmtReader::reset() {
    if (session_) {
        session_->capture.release();
        session_.reset();
    }
    phase_ = Phase::Idle;
}

} // namespace ewfm
