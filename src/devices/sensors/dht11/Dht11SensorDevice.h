#pragma once

#include "devices/sensors/common/PolledTemperatureHumiditySensorDeviceBase.h"
#include "devices/sensors/dht11/Dht11LineDriver.h"
#include "devices/sensors/dht11/Dht11Protocol.h"
#include "devices/sensors/dht11/Dht11RmtReader.h"
#include "devices/sensors/dht11/Dht11SensorConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

// DHT11 is a single-wire GPIO-attached temperature + humidity sensor. The device owns the
// polling lifecycle and publishes temperature + humidity through the shared temp/humidity
// reading publishers; the protocol helper handles the bounded bit capture and checksum decode.
class Dht11SensorDevice final : public PolledTemperatureHumiditySensorDeviceBase {
public:
    Dht11SensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    Dht11SensorDevice(const Dht11SensorConfigV3& config, IDht11LineDriver& lineDriver);

    const Dht11SensorConfigV3& config() const;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;
    TemperatureHumiditySampleResult sampleReading(uint32_t now, int32_t& milliCelsius, int32_t& milliPercent,
                                                  const char*& invalidStatus) override;
    uint32_t pollIntervalMs() const override;
    const SensorFilterConfigV1& temperatureFilterConfig() const override;
    const SensorFilterConfigV1& humidityFilterConfig() const override;
    bool reportAlways() const override;
    uint16_t reportDeltaCentiCelsius() const override;
    uint16_t reportDeltaCentiPercent() const override;
    uint32_t startDelayMs() const override;

    Dht11SensorConfigV3 config_{};
    IDht11LineDriver& lineDriver_;
    Dht11RmtReader rmtReader_{};
};

} // namespace ewfm
