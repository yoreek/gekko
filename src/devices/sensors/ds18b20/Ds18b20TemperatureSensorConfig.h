#pragma once

#include "devices/bus/onewire/OneWireRomAddress.h"
#include "devices/core/DeviceTypes.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>
#include <cstdint>
#include <string>

namespace ewfm {

constexpr DeviceTypeId kDs18b20TemperatureSensorTypeId = 4;
constexpr uint32_t kDs18b20TemperatureSensorConfigVersion = 1;
constexpr uint32_t kDs18b20DefaultPollMs = 5000;
constexpr uint32_t kDs18b20MinPollMs = 1000;
constexpr uint32_t kDs18b20MaxPollMs = 86400000UL;
constexpr uint16_t kDs18b20DefaultReportDeltaCentiCelsius = 1;

#pragma pack(push, 1)
struct Ds18b20TemperatureSensorConfigV1 {
    static constexpr uint32_t kMagicKey = 0x44533138UL;
    uint8_t enabled{1};
    OneWireRomAddress address{};
    uint8_t resolution{12};
    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kDs18b20DefaultReportDeltaCentiCelsius};
    uint32_t pollMs{kDs18b20DefaultPollMs};
};
#pragma pack(pop)

std::string encodeDs18b20TemperatureSensorConfig(const Ds18b20TemperatureSensorConfigV1& config);
bool decodeDs18b20TemperatureSensorConfig(const std::string& blob, Ds18b20TemperatureSensorConfigV1& config);
DeviceValidationResult validateDs18b20TemperatureSensorConfig(const Ds18b20TemperatureSensorConfigV1& config);
bool parseDs18b20TemperatureSensorConfigJson(const JsonObjectConst& input, Ds18b20TemperatureSensorConfigV1& config, std::string& error);
void writeDs18b20TemperatureSensorConfigJson(const Ds18b20TemperatureSensorConfigV1& config, JsonObject output);

} // namespace ewfm
