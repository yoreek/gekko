#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/sensors/filter/SensorReadingFilter.h"
#include "devices/sensors/ntc_thermistor/IAdcInputDriver.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorConfig.h"
#include "devices/sensors/temperature/TemperatureReadingPublisher.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>

namespace ewfm {

class NtcThermistorTemperatureSensorDevice final : public DeviceRuntimeBase, public ITemperatureReadingRuntime {
public:
    NtcThermistorTemperatureSensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit NtcThermistorTemperatureSensorDevice(const NtcThermistorTemperatureSensorConfigV1& config);
    NtcThermistorTemperatureSensorDevice(const NtcThermistorTemperatureSensorConfigV1& config, IAdcInputDriver& driver);

    const NtcThermistorTemperatureSensorConfigV1& config() const;
    const TemperatureReading& reading() const;
    const char* outputStatus() const;
    bool latestTemperatureReading(TemperatureReading& reading) const override;
    const char* latestTemperatureStatus() const override;
    const ITemperatureReadingRuntime* temperatureReadingRuntime() const override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    void end(uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    bool configurePin();
    void releaseHardware(uint32_t now);
    void performReading(uint32_t now);
    int32_t readCentiCelsius() const;
    TemperatureUnit outputUnit() const;

    NtcThermistorTemperatureSensorConfigV1 config_{};
    IAdcInputDriver& driver_;
    SensorReadingFilter filter_{};
    TemperatureReadingPublisher publisher_{};
    uint32_t nextPollAt_{0};
};

} // namespace ewfm
