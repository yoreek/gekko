#pragma once

#include "devices/bus/BusDependentDeviceBase.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/i2c/II2cBusDriver.h"
#include "devices/sensors/filter/SensorFilterConfig.h"
#include "devices/sensors/filter/SensorReadingFilter.h"
#include "devices/sensors/humidity/HumidityReadingPublisher.h"
#include "devices/sensors/humidity/HumiditySensorTypes.h"
#include "devices/sensors/temperature/TemperatureReadingPublisher.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

namespace ewfm {

// Shared runtime skeleton for async I2C dual-channel (temperature+humidity) sensors that need
// an explicit init/reset step followed by one or more trigger->wait->read measurement phases:
// AHT10 (one phase, decodes both channels from a single frame) and HTU21 (two phases, one per
// channel, plus a CRC check). Owns the whole lifecycle state machine, bus-dependency bookkeeping,
// poll cadence, per-channel smoothing filters, and reading publishers -- a concrete sensor
// implements only the protocol hooks (init/trigger/read per phase) and the config-derived policy
// accessors. See docs/device-scaffolding.md ("base classes as templates").
//
// Distinct from PolledTemperatureSensorDeviceBase (synchronous, single channel): this family's
// read model is inherently multi-tick (a real conversion delay that must not hold the I2C bus
// across ticks) and multi-channel, which docs/device-scaffolding.md flagged as needing its own
// base when PolledTemperatureSensorDeviceBase was extracted.
class I2cAsyncTemperatureHumiditySensorDeviceBase : public BusDependentDeviceBase<I2cBusDevice, DeviceRole::I2CBus>,
                                                    public ITemperatureReadingRuntime,
                                                    public IHumidityReadingRuntime {
public:
    I2cAsyncTemperatureHumiditySensorDeviceBase();

    bool latestTemperatureReading(TemperatureReading& reading) const override;
    const char* latestTemperatureStatus() const override;
    bool latestHumidityReading(HumidityReading& reading) const override;
    const char* latestHumidityStatus() const override;
    const ITemperatureReadingRuntime* temperatureReadingRuntime() const override;
    const IHumidityReadingRuntime* humidityReadingRuntime() const override;

protected:
    // ---- hooks a concrete sensor must implement ----
    // Sends the init/reset command. Also responsible for probing chip presence first (e.g. by
    // calling the inherited I2cDeviceRuntimeBase::probeI2cPresence()) since that helper lives one
    // layer above this base in the CRTP stack.
    virtual bool sendInitCommand(II2cBusDriver& driver) const = 0;
    virtual uint32_t initDelayMs() const = 0;
    // Number of trigger->wait->read phases to cycle through per full measurement (AHT10: 1;
    // HTU21: 2 -- one per channel).
    virtual uint8_t measurementPhaseCount() const = 0;
    virtual bool sendPhaseTrigger(II2cBusDriver& driver, uint8_t phaseIndex) const = 0;
    virtual uint32_t phaseDelayMs(uint8_t phaseIndex) const = 0;
    // Reads and decodes phase `phaseIndex`, stashing the raw value(s) into the concrete class's
    // own fields. Return false and set `errorStatus` to a stable status string on failure. A
    // device whose protocol distinguishes "conversion not finished yet" from a real failure may
    // return that status as the literal "busy" (matched by content, not pointer identity, since
    // it may originate in a different translation unit) to get a fast retry that does not count
    // toward the fault threshold.
    virtual bool readPhase(II2cBusDriver& driver, uint8_t phaseIndex, const char*& errorStatus) = 0;
    // Called once after the last phase succeeds; converts the stashed raw value(s) to milli-units.
    virtual void computeReadings(int32_t& milliCelsius, int32_t& milliPercent) const = 0;

    // ---- config-derived policy ----
    virtual uint32_t pollIntervalMs() const = 0;
    virtual const SensorFilterConfigV1& temperatureFilterConfig() const = 0;
    virtual const SensorFilterConfigV1& humidityFilterConfig() const = 0;
    virtual bool reportAlways() const = 0;
    virtual uint16_t reportDeltaCentiCelsius() const = 0;
    virtual uint16_t reportDeltaCentiPercent() const = 0;

    // ---- helpers for a concrete constructor / applyConfig() ----
    void applyFilterConfig();          // re-seed temperatureFilter_/humidityFilter_
    void reschedulePoll(uint32_t now); // nextPollAt_ = now + pollIntervalMs()
    void publishReadings(int32_t rawMilliCelsius, int32_t rawMilliPercent, uint32_t now);
    void invalidateReadings(const char* status);
    void recordFailure(uint32_t now);

private:
    State Idle();
    State Starting();
    State InitDelay();
    State TriggerPhase();
    State ReadPhase();
    State Ready();
    State RetryBackoff();
    State DependencyBlocked();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    SensorReadingFilter temperatureFilter_{};
    SensorReadingFilter humidityFilter_{};
    TemperatureReadingPublisher temperaturePublisher_{};
    HumidityReadingPublisher humidityPublisher_{};
    uint32_t lastDependencyGeneration_{0};
    uint32_t nextPollAt_{0};
    uint32_t retryDeadline_{0};
    uint32_t phaseDeadline_{0};
    uint8_t currentPhase_{0};
    uint8_t consecutiveErrors_{0};
};

} // namespace ewfm
