#pragma once

#include "devices/sensors/common/PolledTemperatureSensorDeviceBase.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

// A pure resistance->temperature calculator over an AnalogInput-role dependency: it owns no ADC
// hardware itself (see AnalogPortInputDevice/AnalogInputChannelDevice for that), only the
// voltage-divider geometry and the Beta/Steinhart-Hart curve. The AnalogInput dependency
// is resolved once per change (setDependencyRuntime/bindDeviceIdentity) rather than re-looked-up
// every tick, mirroring ThermostatDevice's capability cache. The poll loop, smoothing filter,
// reading publisher, and lifecycle state machine all live in PolledTemperatureSensorDeviceBase;
// this class supplies only the divider read and the config accessors.
class NtcThermistorTemperatureSensorDevice final : public PolledTemperatureSensorDeviceBase {
public:
    NtcThermistorTemperatureSensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit NtcThermistorTemperatureSensorDevice(const NtcThermistorTemperatureSensorConfigV1& config);

    const NtcThermistorTemperatureSensorConfigV1& config() const;
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

    // PolledTemperatureSensorDeviceBase hooks
    bool sensorReady() const override;
    bool sampleReading(uint32_t now, int32_t& milliCelsius, const char*& invalidStatus) override;
    uint32_t pollIntervalMs() const override;
    const SensorFilterConfigV1& filterConfig() const override;
    bool reportAlways() const override;
    uint16_t reportDeltaCentiCelsius() const override;

    void refreshCapabilityCache();

    NtcThermistorTemperatureSensorConfigV1 config_{};
    const IAnalogInputRuntime* analogInput_{nullptr};
};

} // namespace ewfm
