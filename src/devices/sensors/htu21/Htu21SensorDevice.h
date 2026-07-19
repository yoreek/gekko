#pragma once

#include "devices/bus/BusDependentDeviceBase.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/i2c/I2cDeviceRuntimeBase.h"
#include "devices/sensors/filter/SensorReadingFilter.h"
#include "devices/sensors/htu21/Htu21SensorConfig.h"
#include "devices/sensors/humidity/HumidityReadingPublisher.h"
#include "devices/sensors/temperature/TemperatureReadingPublisher.h"

#include <ArduinoJson.h>

namespace ewfm {

// HTU21(D)F combined temperature + humidity sensor on a shared I2C bus (default address 0x40).
// One device, two output channels: it provides DeviceRole::TemperatureSensor (so thermostats can
// bind it exactly like a DS18B20) and additionally exposes humidity through the role-less
// IHumidityReadingRuntime for the metric/display pipeline. Bus handling mirrors Ds3231RtcDevice;
// the poll cycle mirrors Ds18b20's split trigger/wait shape because the no-hold measurement
// commands need a conversion delay that must not hold a bus transaction across ticks.
class Htu21SensorDevice;
using Htu21SensorDeviceBase = I2cDeviceRuntimeBase<Htu21SensorDevice, BusDependentDeviceBase<I2cBusDevice, DeviceRole::I2CBus>>;

class Htu21SensorDevice final : public Htu21SensorDeviceBase, public ITemperatureReadingRuntime, public IHumidityReadingRuntime {
public:
    Htu21SensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Htu21SensorDevice(const Htu21SensorConfigV2& config);
    explicit Htu21SensorDevice(const Htu21SensorConfigV3& config);

    const Htu21SensorConfigV3& config() const;
    const ITemperatureReadingRuntime* temperatureReadingRuntime() const override;
    const IHumidityReadingRuntime* humidityReadingRuntime() const override;
    bool latestTemperatureReading(TemperatureReading& reading) const override;
    const char* latestTemperatureStatus() const override;
    bool latestHumidityReading(HumidityReading& reading) const override;
    const char* latestHumidityStatus() const override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    State Idle();
    State Starting();
    State ResetDelay();
    State TriggerTemperature();
    State ReadTemperature();
    State TriggerHumidity();
    State ReadHumidity();
    State Ready();
    State RetryBackoff();
    State DependencyBlocked();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    bool triggerMeasurement(II2cBusDriver& driver, uint8_t command) const;
    bool readMeasurement(II2cBusDriver& driver, uint16_t& raw, const char*& errorStatus) const;
    TemperatureUnit outputUnit() const;
    void publishReadings(int32_t rawMilliCelsius, int32_t rawMilliPercent, uint32_t now);
    void invalidateReadings(const char* status);
    void recordFailure(uint32_t now);

    Htu21SensorConfigV3 config_{};
    SensorReadingFilter temperatureFilter_{};
    SensorReadingFilter humidityFilter_{};
    TemperatureReadingPublisher temperaturePublisher_{};
    HumidityReadingPublisher humidityPublisher_{};
    uint32_t lastDependencyGeneration_{0};
    uint32_t nextPollAt_{0};
    uint32_t retryDeadline_{0};
    uint32_t measureDeadline_{0};
    int32_t pendingMilliCelsius_{0};
    uint8_t consecutiveErrors_{0};
};

} // namespace ewfm
