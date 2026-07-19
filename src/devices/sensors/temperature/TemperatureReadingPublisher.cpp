#include "devices/sensors/temperature/TemperatureReadingPublisher.h"

#include <cstring>

namespace ewfm {

namespace {
constexpr const char* kOutputOk = "ok";
} // namespace

void TemperatureReadingPublisher::configure(bool reportAlways, uint16_t reportDeltaCentiCelsius) {
    reportAlways_ = reportAlways;
    reportDeltaCentiCelsius_ = reportDeltaCentiCelsius;
}

bool TemperatureReadingPublisher::publish(int32_t milliCelsius, uint32_t now) {
    const TemperatureReading previous = reading_;
    reading_.milliCelsius = milliCelsius;
    reading_.measuredAtMs = now;
    reading_.valid = true;
    const bool heartbeatDue = static_cast<uint32_t>(now - lastPublishedAtMs_) >= kHeartbeatIntervalMs;
    const bool changed = reportAlways_ || temperatureReadingChanged(previous, reading_, reportDeltaCentiCelsius_) || heartbeatDue ||
                         std::strcmp(status_, kOutputOk) != 0;
    status_ = kOutputOk;
    if (changed) {
        lastPublishedAtMs_ = now;
    }
    return changed;
}

bool TemperatureReadingPublisher::invalidate(const char* status) {
    const bool changed = reading_.valid || std::strcmp(status_, status) != 0;
    reading_.milliCelsius = 0;
    reading_.measuredAtMs = 0;
    reading_.valid = false;
    status_ = status;
    return changed;
}

const TemperatureReading& TemperatureReadingPublisher::reading() const {
    return reading_;
}

const char* TemperatureReadingPublisher::status() const {
    return status_;
}

void TemperatureReadingPublisher::setStatusQuietly(const char* status) {
    status_ = status;
}

} // namespace ewfm
