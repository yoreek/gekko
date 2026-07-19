#pragma once

#include "devices/sensors/humidity/HumiditySensorTypes.h"

#include <cstdint>

namespace ewfm {

// Owns the "last known reading" plus the dirty-on-change publish semantics for humidity
// channels (deadband via humidityReadingChanged, reportAlways override, republish on status
// change). Composed (has-a) by sensor runtime classes, mirroring TemperatureReadingPublisher.
class HumidityReadingPublisher {
public:
    void configure(bool reportAlways, uint16_t reportDeltaCentiPercent);

    // Returns true if the change is significant enough that the caller should call markRuntimeStateDirty().
    bool publish(int32_t milliPercent, uint32_t now);

    // Returns true if the caller should call markRuntimeStateDirty().
    bool invalidate(const char* status);

    const HumidityReading& reading() const;
    const char* status() const;

    // Overwrites the status label without touching the reading or signalling a dirty state.
    void setStatusQuietly(const char* status);

private:
    // A stable reading that never crosses the deadband would otherwise never republish - force a
    // republish at least this often so observers can tell the sensor is still alive.
    static constexpr uint32_t kHeartbeatIntervalMs = 300000U; // 5 minutes

    HumidityReading reading_{};
    const char* status_{"not_ready"};
    bool reportAlways_{false};
    uint16_t reportDeltaCentiPercent_{0};
    uint32_t lastPublishedAtMs_{0};
};

} // namespace ewfm
