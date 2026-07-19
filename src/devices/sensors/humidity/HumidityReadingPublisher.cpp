#include "devices/sensors/humidity/HumidityReadingPublisher.h"

#include <cstring>

namespace ewfm {

namespace {
constexpr const char* kOutputOk = "ok";
} // namespace

void HumidityReadingPublisher::configure(bool reportAlways, uint16_t reportDeltaCentiPercent) {
    reportAlways_ = reportAlways;
    reportDeltaCentiPercent_ = reportDeltaCentiPercent;
}

bool HumidityReadingPublisher::publish(int32_t milliPercent, uint32_t now) {
    const HumidityReading previous = reading_;
    reading_.milliPercent = milliPercent;
    reading_.measuredAtMs = now;
    reading_.valid = true;
    const bool heartbeatDue = static_cast<uint32_t>(now - lastPublishedAtMs_) >= kHeartbeatIntervalMs;
    const bool changed = reportAlways_ || humidityReadingChanged(previous, reading_, reportDeltaCentiPercent_) || heartbeatDue ||
                         std::strcmp(status_, kOutputOk) != 0;
    status_ = kOutputOk;
    if (changed) {
        lastPublishedAtMs_ = now;
    }
    return changed;
}

bool HumidityReadingPublisher::invalidate(const char* status) {
    const bool changed = reading_.valid || std::strcmp(status_, status) != 0;
    reading_.milliPercent = 0;
    reading_.measuredAtMs = 0;
    reading_.valid = false;
    status_ = status;
    return changed;
}

const HumidityReading& HumidityReadingPublisher::reading() const {
    return reading_;
}

const char* HumidityReadingPublisher::status() const {
    return status_;
}

void HumidityReadingPublisher::setStatusQuietly(const char* status) {
    status_ = status;
}

} // namespace ewfm
