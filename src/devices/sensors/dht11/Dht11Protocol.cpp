#include "devices/sensors/dht11/Dht11Protocol.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace ewfm {

namespace {
constexpr uint32_t kPollStepMicros = 2U;

class Dht11TimingGuard final {
public:
    Dht11TimingGuard() {
#if defined(ARDUINO)
        noInterrupts();
#endif
    }

    ~Dht11TimingGuard() {
#if defined(ARDUINO)
        interrupts();
#endif
    }
};

bool waitForLevel(IDht11LineDriver& driver, uint8_t pin, bool expected, uint32_t timeoutMicros, const char*& errorStatus) {
    for (uint32_t waited = 0U; waited < timeoutMicros; waited += kPollStepMicros) {
        bool level = false;
        if (!driver.read(pin, level)) {
            errorStatus = "not_found";
            return false;
        }
        if (level == expected) {
            return true;
        }
        driver.waitMicros(kPollStepMicros);
    }
    errorStatus = "timeout";
    return false;
}

bool measurePulse(IDht11LineDriver& driver, uint8_t pin, bool expected, uint32_t& pulseMicros, const char*& errorStatus) {
    pulseMicros = 0U;
    for (; pulseMicros < kDht11ResponseTimeoutMicros; pulseMicros += kPollStepMicros) {
        bool level = false;
        if (!driver.read(pin, level)) {
            errorStatus = "not_found";
            return false;
        }
        if (level != expected) {
            return true;
        }
        driver.waitMicros(kPollStepMicros);
    }
    errorStatus = "timeout";
    return false;
}
} // namespace

bool dht11ChecksumValid(const uint8_t* frame) {
    const uint8_t checksum = static_cast<uint8_t>(frame[0] + frame[1] + frame[2] + frame[3]);
    return checksum == frame[4];
}

int32_t dht11RawToMilliCelsius(uint8_t temperatureInteger, uint8_t temperatureDecimal) {
    return static_cast<int32_t>(temperatureInteger) * 1000 + static_cast<int32_t>(temperatureDecimal) * 100;
}

int32_t dht11RawToMilliPercent(uint8_t humidityInteger, uint8_t humidityDecimal) {
    return static_cast<int32_t>(humidityInteger) * 1000 + static_cast<int32_t>(humidityDecimal) * 100;
}

bool dht11DecodeFrame(const uint8_t* frame, int32_t& milliCelsius, int32_t& milliPercent, const char*& errorStatus) {
    if (!dht11ChecksumValid(frame)) {
        errorStatus = "checksum_error";
        return false;
    }
    milliPercent = dht11RawToMilliPercent(frame[0], frame[1]);
    milliCelsius = dht11RawToMilliCelsius(frame[2], frame[3]);
    errorStatus = nullptr;
    return true;
}

bool dht11CaptureMeasurement(IDht11LineDriver& driver, uint8_t pin, bool internalPullup, int32_t& milliCelsius, int32_t& milliPercent,
                             const char*& errorStatus) {
    if (!driver.driveLow(pin)) {
        errorStatus = "not_found";
        return false;
    }
    driver.waitMicros(kDht11StartPulseMicros);
    if (!driver.release(pin, internalPullup)) {
        errorStatus = "not_found";
        return false;
    }
    driver.waitMicros(kDht11ReleaseWaitMicros);

    if (!waitForLevel(driver, pin, false, kDht11ResponseTimeoutMicros, errorStatus)) {
        return false;
    }

    Dht11TimingGuard timingGuard;
    if (!waitForLevel(driver, pin, true, kDht11ResponseTimeoutMicros, errorStatus)) {
        return false;
    }
    if (!waitForLevel(driver, pin, false, kDht11ResponseTimeoutMicros, errorStatus)) {
        return false;
    }

    PulseCaptureSample pulses[kDht11DataPulseCount]{};
    for (uint8_t bitIndex = 0U; bitIndex < 40U; ++bitIndex) {
        uint32_t lowMicros = 0U;
        uint32_t highMicros = 0U;
        if (!measurePulse(driver, pin, false, lowMicros, errorStatus) || !measurePulse(driver, pin, true, highMicros, errorStatus)) {
            return false;
        }
        pulses[bitIndex * 2U] = {false, static_cast<uint16_t>(lowMicros)};
        pulses[bitIndex * 2U + 1U] = {true, static_cast<uint16_t>(highMicros)};
    }
    return dht11DecodePulsePairs(pulses, kDht11DataPulseCount, milliCelsius, milliPercent, errorStatus);
}

} // namespace ewfm
