#pragma once

#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/i2c/I2cDeviceRuntimeBase.h"
#include "devices/sensors/common/I2cAsyncTemperatureHumiditySensorDeviceBase.h"
#include "devices/sensors/htu21/Htu21SensorConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

// HTU21(D)F combined temperature + humidity sensor on a shared I2C bus (default address 0x40).
// One device, two output channels: it provides DeviceRole::TemperatureSensor (so thermostats can
// bind it exactly like a DS18B20) and additionally exposes humidity through the role-less
// IHumidityReadingRuntime for the metric/display pipeline. Runtime lifecycle, filtering, and
// publishing are owned by I2cAsyncTemperatureHumiditySensorDeviceBase; this class supplies only
// the HTU21 protocol (soft reset, two no-hold measurement phases with CRC-checked responses).
class Htu21SensorDevice;
using Htu21SensorDeviceBase = I2cDeviceRuntimeBase<Htu21SensorDevice, I2cAsyncTemperatureHumiditySensorDeviceBase>;

class Htu21SensorDevice final : public Htu21SensorDeviceBase {
public:
    Htu21SensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Htu21SensorDevice(const Htu21SensorConfigV3& config);

    const Htu21SensorConfigV3& config() const;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    bool sendInitCommand(II2cBusDriver& driver) const override;
    uint32_t initDelayMs() const override;
    uint8_t measurementPhaseCount() const override;
    bool sendPhaseTrigger(II2cBusDriver& driver, uint8_t phaseIndex) const override;
    uint32_t phaseDelayMs(uint8_t phaseIndex) const override;
    bool readPhase(II2cBusDriver& driver, uint8_t phaseIndex, const char*& errorStatus) override;
    void computeReadings(int32_t& milliCelsius, int32_t& milliPercent) const override;

    uint32_t pollIntervalMs() const override;
    const SensorFilterConfigV1& temperatureFilterConfig() const override;
    const SensorFilterConfigV1& humidityFilterConfig() const override;
    bool reportAlways() const override;
    uint16_t reportDeltaCentiCelsius() const override;
    uint16_t reportDeltaCentiPercent() const override;

    TemperatureUnit outputUnit() const;

    Htu21SensorConfigV3 config_{};
    int32_t pendingMilliCelsius_{0};
    int32_t pendingMilliPercent_{0};
};

} // namespace ewfm
