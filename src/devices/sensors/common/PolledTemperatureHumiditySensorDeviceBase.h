#pragma once

#include "core/StateMachine.h"
#include "devices/core/DeviceRuntimeBase.h"
#include "devices/sensors/filter/SensorFilterConfig.h"
#include "devices/sensors/filter/SensorReadingFilter.h"
#include "devices/sensors/humidity/HumidityReadingPublisher.h"
#include "devices/sensors/humidity/HumiditySensorTypes.h"
#include "devices/sensors/temperature/TemperatureReadingPublisher.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

namespace ewfm {

class PolledTemperatureHumiditySensorDeviceBase : public DeviceRuntimeBase,
                                                  public ITemperatureReadingRuntime,
                                                  public IHumidityReadingRuntime {
public:
    PolledTemperatureHumiditySensorDeviceBase();

    const TemperatureReading& temperatureReading() const;
    const HumidityReading& humidityReading() const;

    bool latestTemperatureReading(TemperatureReading& reading) const override;
    const char* latestTemperatureStatus() const override;
    bool latestHumidityReading(HumidityReading& reading) const override;
    const char* latestHumidityStatus() const override;
    const ITemperatureReadingRuntime* temperatureReadingRuntime() const override;
    const IHumidityReadingRuntime* humidityReadingRuntime() const override;

protected:
    virtual bool sampleReading(uint32_t now, int32_t& milliCelsius, int32_t& milliPercent, const char*& invalidStatus) = 0;
    virtual uint32_t pollIntervalMs() const = 0;
    virtual const SensorFilterConfigV1& temperatureFilterConfig() const = 0;
    virtual const SensorFilterConfigV1& humidityFilterConfig() const = 0;
    virtual bool reportAlways() const = 0;
    virtual uint16_t reportDeltaCentiCelsius() const = 0;
    virtual uint16_t reportDeltaCentiPercent() const = 0;
    virtual uint32_t startDelayMs() const {
        return 0U;
    }

    void applyFilterConfig();
    void reschedulePoll(uint32_t now);
    void publishReadings(int32_t rawMilliCelsius, int32_t rawMilliPercent, uint32_t now);
    void invalidateReadings(const char* status);
    void recordFailure(uint32_t now);

private:
    void performReading(uint32_t now);

    State Idle();
    State Starting();
    State Ready();
    State RetryBackoff();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    SensorReadingFilter temperatureFilter_{};
    SensorReadingFilter humidityFilter_{};
    TemperatureReadingPublisher temperaturePublisher_{};
    HumidityReadingPublisher humidityPublisher_{};
    uint32_t nextPollAt_{0U};
    uint32_t retryDeadline_{0U};
    uint8_t consecutiveErrors_{0U};
};

} // namespace ewfm
