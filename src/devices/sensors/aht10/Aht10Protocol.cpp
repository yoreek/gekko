#include "devices/sensors/aht10/Aht10Protocol.h"

namespace ewfm {

bool aht10MeasurementReady(uint8_t status) {
    return (status & kAht10StatusBusy) == 0U;
}

bool aht10MeasurementCalibrated(uint8_t status) {
    return (status & kAht10StatusCalibrated) != 0U;
}

bool aht10DecodeMeasurement(const uint8_t* frame, uint32_t& humidityRaw, uint32_t& temperatureRaw, const char*& errorStatus) {
    const uint8_t status = frame[0];
    if (!aht10MeasurementReady(status)) {
        errorStatus = "busy";
        return false;
    }
    if (!aht10MeasurementCalibrated(status)) {
        errorStatus = "uncalibrated";
        return false;
    }

    humidityRaw = (static_cast<uint32_t>(frame[1]) << 12) | (static_cast<uint32_t>(frame[2]) << 4) | (static_cast<uint32_t>(frame[3]) >> 4);
    temperatureRaw =
        (static_cast<uint32_t>(frame[3] & 0x0F) << 16) | (static_cast<uint32_t>(frame[4]) << 8) | static_cast<uint32_t>(frame[5]);
    errorStatus = nullptr;
    return true;
}

int32_t aht10RawToMilliCelsius(uint32_t raw) {
    const int64_t scaled = static_cast<int64_t>(raw) * 200000LL;
    return static_cast<int32_t>(scaled / 1048576LL - 50000LL);
}

int32_t aht10RawToMilliPercent(uint32_t raw) {
    const int64_t scaled = static_cast<int64_t>(raw) * 100000LL;
    int64_t value = scaled / 1048576LL;
    if (value < 0LL) {
        value = 0LL;
    } else if (value > 100000LL) {
        value = 100000LL;
    }
    return static_cast<int32_t>(value);
}

} // namespace ewfm
