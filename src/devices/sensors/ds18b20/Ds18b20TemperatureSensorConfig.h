#pragma once

#include "devices/bus/onewire/OneWireRomAddress.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/sensors/filter/SensorFilterConfig.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kDs18b20TemperatureSensorTypeId = 4;
constexpr uint32_t kDs18b20TemperatureSensorConfigVersion = 2;
constexpr uint32_t kDs18b20DefaultPollMs = 5000;
constexpr uint32_t kDs18b20MinPollMs = 1000;
constexpr uint32_t kDs18b20MaxPollMs = 86400000UL;
constexpr uint16_t kDs18b20DefaultReportDeltaCentiCelsius = 1;

#pragma pack(push, 1)
struct [[deprecated("legacy persisted DS18B20 config; decode/migration only")]] Ds18b20TemperatureSensorConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "DS18B20-1";
    OneWireRomAddress address{};
    uint8_t resolution{12};
    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kDs18b20DefaultReportDeltaCentiCelsius};
    uint32_t pollMs{kDs18b20DefaultPollMs};

    DeviceValidationResult validate() const; // kept for decode/migration only
};

// V2 adds a smoothing/calibration filter, bringing DS18B20 in line with the NTC and HTU21
// sensors. Fields are appended after V1's, under a new kMagic, so an old "DS18B20-1" blob can
// never be misread as V2 -- it is decoded as V1 and migrated (filter defaults to pass-through).
struct Ds18b20TemperatureSensorConfigV2 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "DS18B20-2";
    OneWireRomAddress address{};
    uint8_t resolution{12};
    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kDs18b20DefaultReportDeltaCentiCelsius};
    uint32_t pollMs{kDs18b20DefaultPollMs};
    SensorFilterConfigV1 filter{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const Ds18b20TemperatureSensorConfigV1& legacy);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t ds18b20TemperatureSensorConfigSize(const Ds18b20TemperatureSensorConfigV1&) {
    return sizeof(Ds18b20TemperatureSensorConfigV1::kMagic) - 1U + sizeof(Ds18b20TemperatureSensorConfigV1);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t ds18b20TemperatureSensorConfigSize(const Ds18b20TemperatureSensorConfigV2&) {
    return sizeof(Ds18b20TemperatureSensorConfigV2::kMagic) - 1U + sizeof(Ds18b20TemperatureSensorConfigV2);
}

// Decodes the latest format, migrating an older supported blob in place.
bool decodeDs18b20TemperatureSensorConfig(const uint8_t* blob, size_t size, Ds18b20TemperatureSensorConfigV2& config);

bool parseDs18b20TemperatureSensorConfigJson(const JsonObjectConst& input, Ds18b20TemperatureSensorConfigV2& config, const char*& error);
void writeDs18b20TemperatureSensorConfigJson(const Ds18b20TemperatureSensorConfigV2& config, JsonObject output);

} // namespace ewfm
