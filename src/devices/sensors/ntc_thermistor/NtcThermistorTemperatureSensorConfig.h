#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/sensors/filter/SensorFilterConfig.h"
#include "devices/sensors/ntc_thermistor/NtcCurve.h"
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

// NTC thermistor sensor: a pure resistance->temperature calculator over an AnalogInput-role
// dependency (see docs/analog-input.md). It owns none of the ADC hardware -- that lives in
// whichever AnalogInput device (analog_port_input, ads1115_input, cd74hc4067_input, ...) it
// depends on -- only the divider geometry and the resistance->temperature curve. `kMagic` bumped
// from "NTC-THERM-1" to "NTC-THERM-2" for this layout change: not a formal migration (this sensor
// has no deployed configs to preserve yet), just a guard so an old blob can't silently decode
// into the new field layout by coincidence of size.
#pragma pack(push, 1)
struct NtcThermistorTemperatureSensorConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "NTC-THERM-2";

    uint8_t formulaMode{static_cast<uint8_t>(NtcFormulaMode::Beta)};
    uint16_t seriesResistorOhms{10000};
    uint16_t supplyMilliVolts{3300};
    uint32_t nominalResistanceOhms{100000}; // R0, used in Beta mode
    int16_t nominalTempCentiCelsius{2500};  // T0, used in Beta mode
    uint16_t betaCoefficient{3950};         // used in Beta mode
    float steinhartA{0.0F};                 // used in Steinhart-Hart mode
    float steinhartB{0.0F};
    float steinhartC{0.0F};
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

bool parseNtcThermistorTemperatureSensorConfigJson(const JsonObjectConst& input, NtcThermistorTemperatureSensorConfigV1& config,
                                                   const char*& error);
void writeNtcThermistorTemperatureSensorConfigJson(const NtcThermistorTemperatureSensorConfigV1& config, JsonObject output);

} // namespace ewfm
