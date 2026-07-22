#pragma once

#include "devices/bus/i2c/I2cDeviceConfig.h"
#include "devices/sensors/filter/SensorFilterConfig.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kHtu21SensorTypeId = 17;
constexpr uint32_t kHtu21SensorConfigVersion = 3;
constexpr uint8_t kHtu21DefaultI2cAddress = 0x40;
constexpr uint32_t kHtu21DefaultPollMs = 5000;
constexpr uint32_t kHtu21MinPollMs = 1000;
constexpr uint32_t kHtu21MaxPollMs = 86400000UL;
constexpr uint16_t kHtu21DefaultReportDeltaCentiCelsius = 10;
constexpr uint16_t kHtu21DefaultReportDeltaCentiPercent = 10;

#pragma pack(push, 1)
// Legacy persisted layouts (V1, V2): kept only so old "HTU21-1"/"HTU21-2" blobs can be decoded
// and migrated to V3. Only the data + validate() survive; JSON handling lives on V3.
struct [[deprecated("legacy persisted HTU21 config; decode/migration only")]] Htu21SensorConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "HTU21-1";

    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kHtu21DefaultReportDeltaCentiCelsius};
    uint16_t reportDeltaCentiPercent{kHtu21DefaultReportDeltaCentiPercent};
    uint32_t pollMs{kHtu21DefaultPollMs};
    SensorFilterConfigV1 temperatureFilter{};
    SensorFilterConfigV1 humidityFilter{};

    DeviceValidationResult validate() const;
};

struct [[deprecated("legacy persisted HTU21 config; decode/migration only")]] Htu21SensorConfigV2 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "HTU21-2";

    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kHtu21DefaultReportDeltaCentiCelsius};
    uint16_t reportDeltaCentiPercent{kHtu21DefaultReportDeltaCentiPercent};
    uint32_t pollMs{kHtu21DefaultPollMs};
    SensorFilterConfigV1 temperatureFilter{};
    SensorFilterConfigV1 humidityFilter{};
    uint8_t i2cAddress{kHtu21DefaultI2cAddress};

    DeviceValidationResult validate() const;
};

struct Htu21SensorConfigV3 : I2cDeviceConfigV1 {
    static constexpr char kMagic[] = "HTU21-3";

    Htu21SensorConfigV3() : I2cDeviceConfigV1(kHtu21DefaultI2cAddress) {}

    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kHtu21DefaultReportDeltaCentiCelsius};
    uint16_t reportDeltaCentiPercent{kHtu21DefaultReportDeltaCentiPercent};
    uint32_t pollMs{kHtu21DefaultPollMs};
    SensorFilterConfigV1 temperatureFilter{};
    SensorFilterConfigV1 humidityFilter{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const Htu21SensorConfigV1& legacy);
    void migrateFrom(const Htu21SensorConfigV2& legacy);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t htu21SensorConfigSize(const Htu21SensorConfigV1&) {
    return sizeof(Htu21SensorConfigV1::kMagic) - 1U + sizeof(Htu21SensorConfigV1);
}

constexpr size_t htu21SensorConfigSize(const Htu21SensorConfigV2&) {
    return sizeof(Htu21SensorConfigV2::kMagic) - 1U + sizeof(Htu21SensorConfigV2);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t htu21SensorConfigSize(const Htu21SensorConfigV3&) {
    return sizeof(Htu21SensorConfigV3::kMagic) - 1U + sizeof(Htu21SensorConfigV3);
}

bool decodeHtu21SensorConfig(const uint8_t* blob, size_t size, Htu21SensorConfigV3& config);
bool parseHtu21SensorConfigJson(const JsonObjectConst& input, Htu21SensorConfigV3& config, const char*& error);
void writeHtu21SensorConfigJson(const Htu21SensorConfigV3& config, JsonObject output);

} // namespace ewfm
