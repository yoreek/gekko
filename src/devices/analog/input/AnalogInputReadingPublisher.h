#pragma once

#include "devices/core/DeviceTypes.h"

#include <cstdint>

namespace ewfm {

// Owns the "last known reading" plus the dirty-on-change publish semantics shared by every
// AnalogInput-role device (deadband via analogInputReadingChanged, reportAlways override,
// heartbeat republish). Mirrors TemperatureReadingPublisher; composed (has-a) by each backend's
// runtime class rather than pulled into a shared state-machine base, for the same reason: the ADC
// poll protocol (plain pin read vs. hub-channel arbitration) differs too much between backends to
// share one base.
class AnalogInputReadingPublisher {
public:
    void configure(bool reportAlways, uint16_t reportDeltaMilliVolts);

    // Returns true if the change is significant enough that the caller should call markRuntimeStateDirty().
    bool publish(uint16_t rawCode, int32_t milliVolts, uint32_t now);

    // Returns true if the caller should call markRuntimeStateDirty().
    bool invalidate(const char* status);

    const AnalogInputReading& reading() const;
    const char* status() const;

    // Overwrites the status label without touching the reading or signalling a dirty state.
    void setStatusQuietly(const char* status);

private:
    // A stable reading that never crosses the deadband would otherwise never republish - force a
    // republish at least this often so observers (e.g. Home Assistant's "last updated" timestamp)
    // can tell the input is still alive, distinct from reportAlways_ (which republishes on every
    // poll instead).
    static constexpr uint32_t kHeartbeatIntervalMs = 300000U; // 5 minutes

    AnalogInputReading reading_{};
    const char* status_{"not_ready"};
    bool reportAlways_{false};
    uint16_t reportDeltaMilliVolts_{0};
    uint32_t lastPublishedAtMs_{0};
};

} // namespace ewfm
