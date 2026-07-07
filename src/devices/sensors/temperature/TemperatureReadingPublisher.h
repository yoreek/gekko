#pragma once

#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <cstdint>

namespace ewfm {

// Owns the "last known reading" plus the dirty-on-change publish semantics shared by every
// temperature sensor device (deadband via temperatureReadingChanged, reportAlways override,
// republish on status change). Composed (has-a) by each sensor's runtime class rather than
// pulled into a shared state-machine base class, since sensor state machines otherwise differ
// too much between device families (bus protocol vs. plain ADC poll) to share one base.
class TemperatureReadingPublisher {
public:
    void configure(bool reportAlways, uint16_t reportDeltaCentiCelsius);

    // Returns true if the change is significant enough that the caller should call markRuntimeStateDirty().
    bool publish(int32_t milliCelsius, uint32_t now);

    // Returns true if the caller should call markRuntimeStateDirty().
    bool invalidate(const char* status);

    const TemperatureReading& reading() const;
    const char* status() const;

    // Overwrites the status label without touching the reading or signalling a dirty state.
    // Used for quiet internal bookkeeping (e.g. clearing a transient "busy" label before a retry)
    // where the caller does not want to notify observers.
    void setStatusQuietly(const char* status);

private:
    // A stable reading that never crosses the deadband would otherwise never republish - force a
    // republish at least this often so observers (e.g. Home Assistant's "last updated" timestamp)
    // can tell the sensor is still alive, distinct from reportAlways_ (which republishes on every
    // poll instead).
    static constexpr uint32_t kHeartbeatIntervalMs = 300000U; // 5 minutes

    TemperatureReading reading_{};
    const char* status_{"not_ready"};
    bool reportAlways_{false};
    uint16_t reportDeltaCentiCelsius_{0};
    uint32_t lastPublishedAtMs_{0};
};

} // namespace ewfm
