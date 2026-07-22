#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/sensors/filter/SensorReadingFilter.h"
#include "devices/sensors/temperature/TemperatureReadingPublisher.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

namespace ewfm {

// Shared runtime skeleton for polled temperature sensors. Owns the whole device
// lifecycle state machine (Idle/Starting/Ready/DependencyBlocked/Reconfiguring/
// Disabled/Deleting), the poll cadence, the smoothing filter, and the reading
// publisher. A concrete sensor implements only the device-specific hooks: readiness,
// a single raw sample, and the config-derived poll/filter/report accessors.
//
// This is to the temperature-sensor family what AbstractOutputDevice is to the output
// family -- see docs/device-scaffolding.md ("base classes as templates"). Before this
// base, ntc/ds18b20/htu21 each hand-rolled the same ~130-line state machine.
class PolledTemperatureSensorDeviceBase : public DeviceRuntimeBase, public ITemperatureReadingRuntime {
public:
    PolledTemperatureSensorDeviceBase();

    const TemperatureReading& reading() const;
    const char* outputStatus() const;

    // ITemperatureReadingRuntime
    bool latestTemperatureReading(TemperatureReading& reading) const override;
    const char* latestTemperatureStatus() const override;
    const ITemperatureReadingRuntime* temperatureReadingRuntime() const override;

protected:
    // ---- hooks a concrete sensor must implement ----
    // True when the sensor can produce a reading (hardware present / dependency ready).
    virtual bool sensorReady() const = 0;
    // Produce one raw reading in milli-Celsius. Return false and set `invalidStatus` to a
    // stable status string (e.g. "out_of_range") when a sample cannot be taken. Filtering
    // and change-based publishing are handled by the base.
    virtual bool sampleReading(uint32_t now, int32_t& milliCelsius, const char*& invalidStatus) = 0;
    // Config-derived policy.
    virtual uint32_t pollIntervalMs() const = 0;
    virtual const SensorFilterConfigV1& filterConfig() const = 0;
    virtual bool reportAlways() const = 0;
    virtual uint16_t reportDeltaCentiCelsius() const = 0;

    // ---- helpers for a concrete constructor / applyConfig() ----
    void applyFilterConfig();          // re-seed filter_ from filterConfig()
    void reschedulePoll(uint32_t now); // nextPollAt_ = now + pollIntervalMs()

private:
    void performReading(uint32_t now);

    // Lifecycle states. States that a device can rest in for a long time are split into a
    // one-shot entry action (does the side effect: publish status / mark deleted) and a parked
    // state that only watches for transitions -- so invalidate()/setDeleted() never run on the
    // per-tick path. Idle/DependencyBlocked already do their side effect in the source transition,
    // so they need no split.
    void Idle();
    void Starting();
    void Ready();
    void DependencyBlocked();
    void Reconfiguring();
    void Disabled();       // entry action: publish "disabled" once
    void DisabledParked(); // parked: watch for delete/reconfigure only
    void Deleting();       // entry action: publish terminal status + mark deleted once
    void Deleted();        // terminal parked: does nothing, waits to be reaped

    SensorReadingFilter filter_{};
    TemperatureReadingPublisher publisher_{};
    uint32_t nextPollAt_{0};
};

} // namespace ewfm
