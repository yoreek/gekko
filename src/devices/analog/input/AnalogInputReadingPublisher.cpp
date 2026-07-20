#include "devices/analog/input/AnalogInputReadingPublisher.h"

#include <cstdlib>
#include <cstring>

namespace ewfm {

namespace {
constexpr const char* kOutputOk = "ok";

bool analogInputReadingChanged(const AnalogInputReading& previous, const AnalogInputReading& current, uint16_t reportDeltaMilliVolts) {
    if (previous.valid != current.valid) {
        return true;
    }
    const int32_t delta = current.milliVolts - previous.milliVolts;
    return std::abs(delta) >= static_cast<int32_t>(reportDeltaMilliVolts);
}
} // namespace

void AnalogInputReadingPublisher::configure(bool reportAlways, uint16_t reportDeltaMilliVolts) {
    reportAlways_ = reportAlways;
    reportDeltaMilliVolts_ = reportDeltaMilliVolts;
}

bool AnalogInputReadingPublisher::publish(uint16_t rawCode, int32_t milliVolts, uint32_t now) {
    const AnalogInputReading previous = reading_;
    reading_.rawCode = rawCode;
    reading_.milliVolts = milliVolts;
    reading_.measuredAtMs = now;
    reading_.valid = true;
    const bool heartbeatDue = static_cast<uint32_t>(now - lastPublishedAtMs_) >= kHeartbeatIntervalMs;
    const bool changed = reportAlways_ || analogInputReadingChanged(previous, reading_, reportDeltaMilliVolts_) || heartbeatDue ||
                         std::strcmp(status_, kOutputOk) != 0;
    status_ = kOutputOk;
    if (changed) {
        lastPublishedAtMs_ = now;
    }
    return changed;
}

bool AnalogInputReadingPublisher::invalidate(const char* status) {
    const bool changed = reading_.valid || std::strcmp(status_, status) != 0;
    reading_.rawCode = 0;
    reading_.milliVolts = 0;
    reading_.measuredAtMs = 0;
    reading_.valid = false;
    status_ = status;
    return changed;
}

const AnalogInputReading& AnalogInputReadingPublisher::reading() const {
    return reading_;
}

const char* AnalogInputReadingPublisher::status() const {
    return status_;
}

void AnalogInputReadingPublisher::setStatusQuietly(const char* status) {
    status_ = status;
}

} // namespace ewfm
