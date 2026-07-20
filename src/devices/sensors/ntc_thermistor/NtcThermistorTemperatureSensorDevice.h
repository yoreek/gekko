#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/sensors/filter/SensorReadingFilter.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorConfig.h"
#include "devices/sensors/temperature/TemperatureReadingPublisher.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>

namespace ewfm {

// A pure resistance->temperature calculator over an AnalogInput-role dependency: it owns no ADC
// hardware itself (see AnalogPortInputDevice/AnalogInputChannelDevice for that), only the
// voltage-divider geometry and the Beta/Steinhart-Hart curve. The AnalogInput dependency
// is resolved once per change (setDependencyRuntime/bindDeviceIdentity) rather than re-looked-up
// every tick, mirroring ThermostatDevice's capability cache.
class NtcThermistorTemperatureSensorDevice final : public DeviceRuntimeBase, public ITemperatureReadingRuntime {
public:
    NtcThermistorTemperatureSensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit NtcThermistorTemperatureSensorDevice(const NtcThermistorTemperatureSensorConfigV1& config);

    const NtcThermistorTemperatureSensorConfigV1& config() const;
    const TemperatureReading& reading() const;
    const char* outputStatus() const;
    bool latestTemperatureReading(TemperatureReading& reading) const override;
    const char* latestTemperatureStatus() const override;
    const ITemperatureReadingRuntime* temperatureReadingRuntime() const override;
    void setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) override;
    void setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    State Idle();
    State Starting();
    State Ready();
    State DependencyBlocked();
    State Reconfiguring();
    State Disabled();
    State Deleting();

    void refreshCapabilityCache();
    bool dependencyAnalogInputReady() const;
    void performReading(uint32_t now);
    TemperatureUnit outputUnit() const;

    NtcThermistorTemperatureSensorConfigV1 config_{};
    SensorReadingFilter filter_{};
    TemperatureReadingPublisher publisher_{};
    const IAnalogInputRuntime* analogInput_{nullptr};
    uint32_t nextPollAt_{0};
};

} // namespace ewfm
