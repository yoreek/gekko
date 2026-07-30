#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/sensors/filter/SensorFilterConfig.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kDht11SensorTypeId = 31;
constexpr uint32_t kDht11SensorConfigVersion = 3;
constexpr uint8_t kDht11DefaultGpioPin = 17;
constexpr uint32_t kDht11DefaultPollMs = 5000;
constexpr uint32_t kDht11MinPollMs = 1000;
constexpr uint32_t kDht11MaxPollMs = 86400000UL;
constexpr uint16_t kDht11DefaultReportDeltaCentiCelsius = 10;
constexpr uint16_t kDht11DefaultReportDeltaCentiPercent = 10;

#pragma pack(push, 1)
struct [[deprecated("legacy persisted DHT11 config; decode/migration only")]] Dht11SensorConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "DHT11-1";

    uint8_t gpioPin{kDht11DefaultGpioPin};
    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kDht11DefaultReportDeltaCentiCelsius};
    uint16_t reportDeltaCentiPercent{kDht11DefaultReportDeltaCentiPercent};
    uint32_t pollMs{kDht11DefaultPollMs};
    SensorFilterConfigV1 temperatureFilter{};
    SensorFilterConfigV1 humidityFilter{};

    DeviceValidationResult validate() const;
};

struct [[deprecated("legacy persisted DHT11 config; decode/migration only")]] Dht11SensorConfigV2 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "DHT11-2";

    uint8_t gpioPin{kDht11DefaultGpioPin};
    uint8_t internalPullup{0};
    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kDht11DefaultReportDeltaCentiCelsius};
    uint16_t reportDeltaCentiPercent{kDht11DefaultReportDeltaCentiPercent};
    uint32_t pollMs{kDht11DefaultPollMs};
    SensorFilterConfigV1 temperatureFilter{};
    SensorFilterConfigV1 humidityFilter{};

    DeviceValidationResult validate() const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const Dht11SensorConfigV1& legacy);
    EWFM_LEGACY_CONFIG_USE_END
};

enum class Dht11CaptureMode : uint8_t {
    Native = 0U,
    Rmt = 1U,
};

struct Dht11SensorConfigV3 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "DHT11-3";

    uint8_t gpioPin{kDht11DefaultGpioPin};
    uint8_t internalPullup{0};
    uint8_t captureMode{static_cast<uint8_t>(Dht11CaptureMode::Native)};
    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kDht11DefaultReportDeltaCentiCelsius};
    uint16_t reportDeltaCentiPercent{kDht11DefaultReportDeltaCentiPercent};
    uint32_t pollMs{kDht11DefaultPollMs};
    SensorFilterConfigV1 temperatureFilter{};
    SensorFilterConfigV1 humidityFilter{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const Dht11SensorConfigV2& legacy);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t dht11SensorConfigSize(const Dht11SensorConfigV1&) {
    return sizeof(Dht11SensorConfigV1::kMagic) - 1U + sizeof(Dht11SensorConfigV1);
}
EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t dht11SensorConfigSize(const Dht11SensorConfigV2&) {
    return sizeof(Dht11SensorConfigV2::kMagic) - 1U + sizeof(Dht11SensorConfigV2);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t dht11SensorConfigSize(const Dht11SensorConfigV3&) {
    return sizeof(Dht11SensorConfigV3::kMagic) - 1U + sizeof(Dht11SensorConfigV3);
}

bool decodeDht11SensorConfig(const uint8_t* blob, size_t size, Dht11SensorConfigV3& config);
bool parseDht11SensorConfigJson(const JsonObjectConst& input, Dht11SensorConfigV3& config, const char*& error);
void writeDht11SensorConfigJson(const Dht11SensorConfigV3& config, JsonObject output);

} // namespace ewfm
