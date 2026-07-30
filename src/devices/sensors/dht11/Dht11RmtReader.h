#pragma once

#include "devices/pulse/RmtPulseCapture.h"
#include "devices/sensors/dht11/Dht11LineDriver.h"
#include "devices/sensors/dht11/Dht11PulseDecoder.h"

#include <cstdint>

namespace ewfm {

// DHT11-specific asynchronous transaction around the reusable RMT pulse receiver.
class Dht11RmtReader final {
public:
    enum class Result : uint8_t {
        Pending,
        Ready,
        Failed,
        Unsupported,
    };

    Result poll(IDht11LineDriver& lineDriver, uint8_t pin, bool internalPullup, uint32_t now, int32_t& milliCelsius, int32_t& milliPercent,
                const char*& errorStatus);
    void cancel();
    void reset();

private:
    enum class Phase : uint8_t {
        Idle,
        StartLow,
        Capturing,
    };

    Phase phase_{Phase::Idle};
    uint32_t startLowAt_{0U};
    RmtPulseCapture capture_{};
    PulseCaptureSample pulses_[kDht11DataPulseCount + 2U]{};
};

} // namespace ewfm
