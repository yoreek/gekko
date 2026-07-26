#pragma once

#include "devices/bus/i2c/I2cDeviceConfig.h"
#include "devices/sensors/filter/SensorFilterConfig.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kAht10SensorTypeId = 29;
constexpr uint32_t kAht10SensorConfigVersion = 1;
constexpr uint8_t kAht10DefaultI2cAddress = 0x38;
constexpr uint32_t kAht10DefaultPollMs = 5000;
constexpr uint32_t kAht10MinPollMs = 1000;
constexpr uint32_t kAht10MaxPollMs = 86400000UL;
constexpr uint16_t kAht10DefaultReportDeltaCentiCelsius = 10;
constexpr uint16_t kAht10DefaultReportDeltaCentiPercent = 10;

#pragma pack(push, 1)
struct Aht10SensorConfigV1 : I2cDeviceConfigV1 {
    static constexpr char kMagic[] = "AHT10-1";

    Aht10SensorConfigV1() : I2cDeviceConfigV1(kAht10DefaultI2cAddress) {}

    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kAht10DefaultReportDeltaCentiCelsius};
    uint16_t reportDeltaCentiPercent{kAht10DefaultReportDeltaCentiPercent};
    uint32_t pollMs{kAht10DefaultPollMs};
    SensorFilterConfigV1 temperatureFilter{};
    SensorFilterConfigV1 humidityFilter{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t aht10SensorConfigSize(const Aht10SensorConfigV1&) {
    return sizeof(Aht10SensorConfigV1::kMagic) - 1U + sizeof(Aht10SensorConfigV1);
}

bool decodeAht10SensorConfig(const uint8_t* blob, size_t size, Aht10SensorConfigV1& config);
bool parseAht10SensorConfigJson(const JsonObjectConst& input, Aht10SensorConfigV1& config, const char*& error);
void writeAht10SensorConfigJson(const Aht10SensorConfigV1& config, JsonObject output);

} // namespace ewfm
