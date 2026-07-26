#include "devices/sensors/dht11/Dht11Protocol.h"

namespace ewfm {

namespace {
constexpr uint32_t kPollStepMicros = 2U;

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

bool readBit(IDht11LineDriver& driver, uint8_t pin, uint8_t& byte, uint8_t bitMask, const char*& errorStatus) {
    if (!waitForLevel(driver, pin, false, kDht11ResponseTimeoutMicros, errorStatus)) {
        return false;
    }
    if (!waitForLevel(driver, pin, true, kDht11ResponseTimeoutMicros, errorStatus)) {
        return false;
    }

    uint32_t highMicros = 0U;
    for (; highMicros < (kDht11ResponseTimeoutMicros * 4U); highMicros += kPollStepMicros) {
        bool level = false;
        if (!driver.read(pin, level)) {
            errorStatus = "not_found";
            return false;
        }
        if (!level) {
            break;
        }
        driver.waitMicros(kPollStepMicros);
    }
    if (highMicros >= (kDht11ResponseTimeoutMicros * 4U)) {
        errorStatus = "timeout";
        return false;
    }
    if (highMicros > kDht11BitThresholdMicros) {
        byte |= bitMask;
    }
    return true;
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

bool dht11CaptureMeasurement(IDht11LineDriver& driver, uint8_t pin, int32_t& milliCelsius, int32_t& milliPercent,
                             const char*& errorStatus) {
    if (!driver.driveLow(pin)) {
        errorStatus = "not_found";
        return false;
    }
    driver.waitMicros(kDht11StartPulseMicros);
    if (!driver.release(pin)) {
        errorStatus = "not_found";
        return false;
    }
    driver.waitMicros(kDht11ReleaseWaitMicros);

    if (!waitForLevel(driver, pin, false, kDht11ResponseTimeoutMicros, errorStatus)) {
        return false;
    }
    if (!waitForLevel(driver, pin, true, kDht11ResponseTimeoutMicros, errorStatus)) {
        return false;
    }

    uint8_t frame[kDht11FrameBytes]{};
    for (uint8_t bitIndex = 0U; bitIndex < 40U; ++bitIndex) {
        const uint8_t byteIndex = bitIndex / 8U;
        const uint8_t bitMask = static_cast<uint8_t>(1U << (7U - (bitIndex % 8U)));
        if (!readBit(driver, pin, frame[byteIndex], bitMask, errorStatus)) {
            return false;
        }
    }
    return dht11DecodeFrame(frame, milliCelsius, milliPercent, errorStatus);
}

} // namespace ewfm
