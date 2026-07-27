#pragma once

#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/i2c/I2cDeviceRuntimeBase.h"
#include "devices/sensors/aht10/Aht10SensorConfig.h"
#include "devices/sensors/common/I2cAsyncTemperatureHumiditySensorDeviceBase.h"

#include <ArduinoJson.h>

namespace ewfm {

// AHT10 combined temperature + humidity sensor on a shared I2C bus (default address 0x38). It
// exposes temperature as the primary temperature_sensor role and humidity through the role-less
// metric pipeline. Runtime lifecycle, filtering, and publishing are owned by
// I2cAsyncTemperatureHumiditySensorDeviceBase; this class supplies only the AHT10 protocol
// (single-phase init+measure command, 6-byte frame decoding both channels at once).
class Aht10SensorDevice;
using Aht10SensorDeviceBase = I2cDeviceRuntimeBase<Aht10SensorDevice, I2cAsyncTemperatureHumiditySensorDeviceBase>;

class Aht10SensorDevice final : public Aht10SensorDeviceBase {
public:
    Aht10SensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Aht10SensorDevice(const Aht10SensorConfigV1& config);

    const Aht10SensorConfigV1& config() const;
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

    Aht10SensorConfigV1 config_{};
    uint32_t humidityRaw_{0};
    uint32_t temperatureRaw_{0};
};

} // namespace ewfm
