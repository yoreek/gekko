#pragma once

#include "devices/bus/onewire/OneWireRomAddress.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kDs18b20TemperatureSensorTypeId = 4;
constexpr uint32_t kDs18b20TemperatureSensorConfigVersion = 1;
constexpr uint32_t kDs18b20DefaultPollMs = 5000;
constexpr uint32_t kDs18b20MinPollMs = 1000;
constexpr uint32_t kDs18b20MaxPollMs = 86400000UL;
constexpr uint16_t kDs18b20DefaultReportDeltaCentiCelsius = 1;

#pragma pack(push, 1)
struct Ds18b20TemperatureSensorConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "DS18B20-1";
    OneWireRomAddress address{};
    uint8_t resolution{12};
    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kDs18b20DefaultReportDeltaCentiCelsius};
    uint32_t pollMs{kDs18b20DefaultPollMs};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t ds18b20TemperatureSensorConfigSize(const Ds18b20TemperatureSensorConfigV1&) {
    return sizeof(Ds18b20TemperatureSensorConfigV1::kMagic) - 1U + sizeof(Ds18b20TemperatureSensorConfigV1);
}

bool parseDs18b20TemperatureSensorConfigJson(const JsonObjectConst& input, Ds18b20TemperatureSensorConfigV1& config, const char*& error);
void writeDs18b20TemperatureSensorConfigJson(const Ds18b20TemperatureSensorConfigV1& config, JsonObject output);

} // namespace ewfm
