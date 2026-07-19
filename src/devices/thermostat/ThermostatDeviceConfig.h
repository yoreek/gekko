#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kThermostatDeviceTypeId = 5;
constexpr uint32_t kThermostatDeviceConfigVersion = 1;
constexpr uint32_t kThermostatDefaultTargetMilliCelsius = 25000;
constexpr uint32_t kThermostatDefaultMinSafeMilliCelsius = 0;
constexpr uint32_t kThermostatDefaultMaxSafeMilliCelsius = 50000;
constexpr uint16_t kThermostatDefaultHysteresisCentiCelsius = 50;
constexpr uint32_t kThermostatDefaultCheckIntervalMs = 1000;
constexpr uint32_t kThermostatDefaultSensorTimeoutMs = 3000;
constexpr uint32_t kThermostatDefaultRetryAfterErrorMs = 30000;
constexpr uint32_t kThermostatDefaultMinSwitchIntervalMs = 5000;
constexpr uint32_t kThermostatMinIntervalMs = 100;
constexpr uint32_t kThermostatMaxIntervalMs = 86400000UL;
constexpr uint16_t kThermostatMaxHysteresisCentiCelsius = 10000;

enum class ThermostatMode : uint8_t {
    Off = 0,
    Cool = 1,
    Heat = 2,
};

enum class ThermostatAlgorithm : uint8_t {
    Hysteresis = 1,
};

#pragma pack(push, 1)
struct ThermostatDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "THRM-1";
    uint8_t mode{static_cast<uint8_t>(ThermostatMode::Off)};
    uint8_t algorithm{static_cast<uint8_t>(ThermostatAlgorithm::Hysteresis)};
    int32_t targetMilliCelsius{kThermostatDefaultTargetMilliCelsius};
    int32_t minSafeMilliCelsius{kThermostatDefaultMinSafeMilliCelsius};
    int32_t maxSafeMilliCelsius{kThermostatDefaultMaxSafeMilliCelsius};
    uint16_t hysteresisCentiCelsius{kThermostatDefaultHysteresisCentiCelsius};
    uint32_t checkIntervalMs{kThermostatDefaultCheckIntervalMs};
    uint32_t sensorTimeoutMs{kThermostatDefaultSensorTimeoutMs};
    uint32_t retryAfterErrorMs{kThermostatDefaultRetryAfterErrorMs};
    uint32_t minSwitchIntervalMs{kThermostatDefaultMinSwitchIntervalMs};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t thermostatDeviceConfigSize(const ThermostatDeviceConfigV1&) {
    return sizeof(ThermostatDeviceConfigV1::kMagic) - 1U + sizeof(ThermostatDeviceConfigV1);
}

bool thermostatModeFromString(const char* value, ThermostatMode& mode);
const char* thermostatModeName(ThermostatMode mode);
bool thermostatAlgorithmFromString(const char* value, ThermostatAlgorithm& algorithm);
const char* thermostatAlgorithmName(ThermostatAlgorithm algorithm);

bool parseThermostatDeviceConfigJson(const JsonObjectConst& input, ThermostatDeviceConfigV1& config, const char*& error);
void writeThermostatDeviceConfigJson(const ThermostatDeviceConfigV1& config, JsonObject output);

} // namespace ewfm
