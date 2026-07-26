#pragma once

#include "devices/bus/BusDependentDeviceBase.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/i2c/I2cDeviceRuntimeBase.h"
#include "devices/sensors/aht10/Aht10SensorConfig.h"
#include "devices/sensors/filter/SensorReadingFilter.h"
#include "devices/sensors/humidity/HumidityReadingPublisher.h"
#include "devices/sensors/temperature/TemperatureReadingPublisher.h"

#include <ArduinoJson.h>

namespace ewfm {

// AHT10 combined temperature + humidity sensor on a shared I2C bus (default address 0x38).
// It exposes temperature as the primary temperature_sensor role and humidity through the
// role-less metric pipeline. The runtime keeps init, trigger, and read phases as explicit
// states so the I2C bus is only held for the transaction itself.
class Aht10SensorDevice;
using Aht10SensorDeviceBase = I2cDeviceRuntimeBase<Aht10SensorDevice, BusDependentDeviceBase<I2cBusDevice, DeviceRole::I2CBus>>;

class Aht10SensorDevice final : public Aht10SensorDeviceBase, public ITemperatureReadingRuntime, public IHumidityReadingRuntime {
public:
    Aht10SensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Aht10SensorDevice(const Aht10SensorConfigV1& config);

    const Aht10SensorConfigV1& config() const;
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
    State InitDelay();
    State TriggerMeasurement();
    State ReadMeasurement();
    State Ready();
    State RetryBackoff();
    State DependencyBlocked();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    bool sendCommand(II2cBusDriver& driver, uint8_t command, uint8_t arg1, uint8_t arg2) const;
    bool readMeasurement(II2cBusDriver& driver, uint32_t& humidityRaw, uint32_t& temperatureRaw, const char*& errorStatus) const;
    TemperatureUnit outputUnit() const;
    void publishReadings(int32_t rawMilliCelsius, int32_t rawMilliPercent, uint32_t now);
    void invalidateReadings(const char* status);
    void recordFailure(uint32_t now);

    Aht10SensorConfigV1 config_{};
    SensorReadingFilter temperatureFilter_{};
    SensorReadingFilter humidityFilter_{};
    TemperatureReadingPublisher temperaturePublisher_{};
    HumidityReadingPublisher humidityPublisher_{};
    uint32_t lastDependencyGeneration_{0};
    uint32_t nextPollAt_{0};
    uint32_t retryDeadline_{0};
    uint32_t initDeadline_{0};
    uint32_t measureDeadline_{0};
    uint8_t consecutiveErrors_{0};
};

} // namespace ewfm
