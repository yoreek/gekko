#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/sensors/filter/SensorFilterConfig.h"
#include "devices/sensors/ntc_thermistor/IAdcInputDriver.h"
#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kNtcThermistorTemperatureSensorTypeId = 10;
constexpr uint32_t kNtcThermistorTemperatureSensorConfigVersion = 1;
constexpr uint32_t kNtcThermistorDefaultPollMs = 5000;
constexpr uint32_t kNtcThermistorMinPollMs = 1000;
constexpr uint32_t kNtcThermistorMaxPollMs = 86400000UL;
constexpr uint16_t kNtcThermistorDefaultReportDeltaCentiCelsius = 10;
constexpr uint8_t kNtcThermistorMinAdcSamples = 1;
constexpr uint8_t kNtcThermistorMaxAdcSamples = 64;

#pragma pack(push, 1)
struct NtcThermistorTemperatureSensorConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "NTC-THERM-1";

    uint8_t gpioPin{34};
    uint8_t attenuation{static_cast<uint8_t>(AdcAttenuation::Db11)};
    uint16_t seriesResistorOhms{10000};
    uint32_t nominalResistanceOhms{100000};
    int16_t nominalTempCentiCelsius{2500};
    uint16_t betaCoefficient{3950};
    uint8_t adcSamples{8};
    uint8_t outputUnit{static_cast<uint8_t>(TemperatureUnit::Celsius)};
    uint8_t reportAlways{0};
    uint16_t reportDeltaCentiCelsius{kNtcThermistorDefaultReportDeltaCentiCelsius};
    uint32_t pollMs{kNtcThermistorDefaultPollMs};
    SensorFilterConfigV1 filter{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t ntcThermistorTemperatureSensorConfigSize(const NtcThermistorTemperatureSensorConfigV1&) {
    return sizeof(NtcThermistorTemperatureSensorConfigV1::kMagic) - 1U + sizeof(NtcThermistorTemperatureSensorConfigV1);
}

bool ntcThermistorGpioPinIsValid(uint8_t pin);
bool attenuationFromByte(uint8_t value, AdcAttenuation& attenuation);
bool attenuationFromString(const char* value, AdcAttenuation& attenuation);
const char* attenuationName(AdcAttenuation attenuation);
bool encodeNtcThermistorTemperatureSensorConfig(const NtcThermistorTemperatureSensorConfigV1& config, uint8_t* blob, size_t capacity);
bool decodeNtcThermistorTemperatureSensorConfig(const uint8_t* blob, size_t size, NtcThermistorTemperatureSensorConfigV1& config);
DeviceValidationResult validateNtcThermistorTemperatureSensorConfig(const NtcThermistorTemperatureSensorConfigV1& config);
bool parseNtcThermistorTemperatureSensorConfigJson(const JsonObjectConst& input, NtcThermistorTemperatureSensorConfigV1& config,
                                                   const char*& error);
void writeNtcThermistorTemperatureSensorConfigJson(const NtcThermistorTemperatureSensorConfigV1& config, JsonObject output);

} // namespace ewfm
