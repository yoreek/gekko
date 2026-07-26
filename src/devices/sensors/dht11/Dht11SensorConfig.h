#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/sensors/filter/SensorFilterConfig.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kDht11SensorTypeId = 31;
constexpr uint32_t kDht11SensorConfigVersion = 1;
constexpr uint8_t kDht11DefaultGpioPin = 17;
constexpr uint32_t kDht11DefaultPollMs = 5000;
constexpr uint32_t kDht11MinPollMs = 1000;
constexpr uint32_t kDht11MaxPollMs = 86400000UL;
constexpr uint16_t kDht11DefaultReportDeltaCentiCelsius = 10;
constexpr uint16_t kDht11DefaultReportDeltaCentiPercent = 10;

#pragma pack(push, 1)
struct Dht11SensorConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "DHT11-1";

    uint8_t gpioPin{kDht11DefaultGpioPin};
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
};
#pragma pack(pop)

constexpr size_t dht11SensorConfigSize(const Dht11SensorConfigV1&) {
    return sizeof(Dht11SensorConfigV1::kMagic) - 1U + sizeof(Dht11SensorConfigV1);
}

bool decodeDht11SensorConfig(const uint8_t* blob, size_t size, Dht11SensorConfigV1& config);
bool parseDht11SensorConfigJson(const JsonObjectConst& input, Dht11SensorConfigV1& config, const char*& error);
void writeDht11SensorConfigJson(const Dht11SensorConfigV1& config, JsonObject output);

} // namespace ewfm
