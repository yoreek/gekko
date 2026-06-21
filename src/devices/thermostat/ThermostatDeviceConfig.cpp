#include "devices/thermostat/ThermostatDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cmath>
#include <cstring>
#include <type_traits>

namespace ewfm {

bool thermostatModeFromByte(uint8_t value, ThermostatMode& mode);
bool thermostatAlgorithmFromByte(uint8_t value, ThermostatAlgorithm& algorithm);

namespace {
bool parseMode(const JsonVariantConst& variant, ThermostatMode& mode, const char*& error) {
    if (variant.isNull()) {
        mode = ThermostatMode::Off;
        return true;
    }
    if (!variant.is<const char*>()) {
        error = "thermostat mode must be a string";
        return false;
    }
    if (!thermostatModeFromString(variant.as<const char*>(), mode)) {
        error = "thermostat mode is invalid";
        return false;
    }
    return true;
}

bool parseAlgorithm(const JsonVariantConst& variant, ThermostatAlgorithm& algorithm, const char*& error) {
    if (variant.isNull()) {
        algorithm = ThermostatAlgorithm::Hysteresis;
        return true;
    }
    if (!variant.is<const char*>()) {
        error = "thermostat algorithm must be a string";
        return false;
    }
    if (!thermostatAlgorithmFromString(variant.as<const char*>(), algorithm)) {
        error = "thermostat algorithm is invalid";
        return false;
    }
    return true;
}

bool parseTemperatureMilliCelsius(const JsonVariantConst& variant, int32_t& milliCelsius, const char*& error) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<float>() && !variant.is<double>() && !variant.is<int>() && !variant.is<long>()) {
        error = "temperature value must be numeric";
        return false;
    }
    const double celsius = variant.as<double>();
    const double milli = std::round(celsius * 1000.0);
    if (milli < static_cast<double>(INT32_MIN) || milli > static_cast<double>(INT32_MAX)) {
        error = "temperature value is out of range";
        return false;
    }
    milliCelsius = static_cast<int32_t>(milli);
    return true;
}

bool parseDuration(const JsonVariantConst& variant, uint32_t& value, const char*& error) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<unsigned long>() && !variant.is<unsigned int>() && !variant.is<int>() && !variant.is<long>()) {
        error = "timing value must be numeric";
        return false;
    }
    const long parsed = variant.as<long>();
    if (parsed < 0) {
        error = "timing value must be non-negative";
        return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool parseMilliCelsiusField(const JsonVariantConst& variant, int32_t& milliCelsius, const char*& error) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<float>() && !variant.is<double>() && !variant.is<int>() && !variant.is<long>() && !variant.is<unsigned long>() &&
        !variant.is<unsigned int>()) {
        error = "temperature value must be numeric";
        return false;
    }
    const double milli = variant.as<double>();
    if (milli < static_cast<double>(INT32_MIN) || milli > static_cast<double>(INT32_MAX)) {
        error = "temperature value is out of range";
        return false;
    }
    milliCelsius = static_cast<int32_t>(std::round(milli));
    return true;
}

bool parseTemperatureField(const JsonObjectConst& input, const char* milliKey, const char* celsiusKey, int32_t& milliCelsius,
                           const char*& error) {
    const JsonVariantConst milliVariant = input[milliKey];
    if (!milliVariant.isNull()) {
        return parseMilliCelsiusField(milliVariant, milliCelsius, error);
    }
    return parseTemperatureMilliCelsius(input[celsiusKey], milliCelsius, error);
}

bool parseHysteresisField(const JsonObjectConst& input, uint16_t& centiCelsius, const char*& error) {
    const JsonVariantConst centiVariant = input["hysteresis_centi_celsius"];
    if (!centiVariant.isNull()) {
        if (!centiVariant.is<float>() && !centiVariant.is<double>() && !centiVariant.is<int>() && !centiVariant.is<long>()) {
            error = "thermostat hysteresis must be numeric";
            return false;
        }
        const long parsed = centiVariant.as<long>();
        if (parsed <= 0 || parsed > static_cast<long>(kThermostatMaxHysteresisCentiCelsius)) {
            error = "thermostat hysteresis is invalid";
            return false;
        }
        centiCelsius = static_cast<uint16_t>(parsed);
        return true;
    }

    const JsonVariantConst celsiusVariant = input["hysteresis_celsius"];
    if (celsiusVariant.isNull()) {
        return true;
    }
    if (!celsiusVariant.is<float>() && !celsiusVariant.is<double>() && !celsiusVariant.is<int>() && !celsiusVariant.is<long>()) {
        error = "thermostat hysteresis must be numeric";
        return false;
    }
    const double hysteresis = celsiusVariant.as<double>();
    if (hysteresis <= 0.0 || hysteresis > static_cast<double>(kThermostatMaxHysteresisCentiCelsius) / 100.0) {
        error = "thermostat hysteresis is invalid";
        return false;
    }
    centiCelsius = static_cast<uint16_t>(std::round(hysteresis * 100.0));
    return centiCelsius != 0U;
}
} // namespace

static_assert(std::is_trivially_copyable<ThermostatDeviceConfigV1>::value, "ThermostatDeviceConfigV1 must be POD");
static_assert(sizeof(ThermostatDeviceConfigV1) <= kMaxDeviceConfigBytes, "ThermostatDeviceConfigV1 exceeds device config bound");

bool thermostatModeFromString(const char* value, ThermostatMode& mode) {
    if (value == nullptr || std::strcmp(value, "off") == 0) {
        mode = ThermostatMode::Off;
        return true;
    }
    if (std::strcmp(value, "heat") == 0) {
        mode = ThermostatMode::Heat;
        return true;
    }
    if (std::strcmp(value, "cool") == 0) {
        mode = ThermostatMode::Cool;
        return true;
    }
    return false;
}

const char* thermostatModeName(ThermostatMode mode) {
    switch (mode) {
    case ThermostatMode::Off:
        return "off";
    case ThermostatMode::Cool:
        return "cool";
    case ThermostatMode::Heat:
        return "heat";
    }
    return "off";
}

bool thermostatAlgorithmFromString(const char* value, ThermostatAlgorithm& algorithm) {
    if (value == nullptr || std::strcmp(value, "hysteresis") == 0) {
        algorithm = ThermostatAlgorithm::Hysteresis;
        return true;
    }
    return false;
}

const char* thermostatAlgorithmName(ThermostatAlgorithm algorithm) {
    switch (algorithm) {
    case ThermostatAlgorithm::Hysteresis:
        return "hysteresis";
    }
    return "hysteresis";
}

bool thermostatModeFromByte(uint8_t value, ThermostatMode& mode) {
    switch (value) {
    case static_cast<uint8_t>(ThermostatMode::Off):
        mode = ThermostatMode::Off;
        return true;
    case static_cast<uint8_t>(ThermostatMode::Cool):
        mode = ThermostatMode::Cool;
        return true;
    case static_cast<uint8_t>(ThermostatMode::Heat):
        mode = ThermostatMode::Heat;
        return true;
    default:
        return false;
    }
}

bool thermostatAlgorithmFromByte(uint8_t value, ThermostatAlgorithm& algorithm) {
    switch (value) {
    case static_cast<uint8_t>(ThermostatAlgorithm::Hysteresis):
        algorithm = ThermostatAlgorithm::Hysteresis;
        return true;
    default:
        return false;
    }
}

bool encodeThermostatDeviceConfig(const ThermostatDeviceConfigV1& config, uint8_t* blob, size_t capacity) {
    return encodeFixedConfigBlob(ThermostatDeviceConfigV1::kMagic, config, blob, capacity);
}

bool decodeThermostatDeviceConfig(const uint8_t* blob, size_t size, ThermostatDeviceConfigV1& config) {
    return decodeFixedConfigBlob(ThermostatDeviceConfigV1::kMagic, blob, size, config) && validateThermostatDeviceConfig(config).ok();
}

DeviceValidationResult validateThermostatDeviceConfig(const ThermostatDeviceConfigV1& config) {
    const DeviceValidationResult baseValidation = validateDeviceBaseConfig(config.base);
    if (!baseValidation.ok()) {
        return baseValidation;
    }

    ThermostatMode mode{};
    ThermostatAlgorithm algorithm{};
    if (!thermostatModeFromByte(config.mode, mode)) {
        return {DeviceError::InvalidConfig, "thermostat mode is invalid"};
    }
    if (!thermostatAlgorithmFromByte(config.algorithm, algorithm)) {
        return {DeviceError::InvalidConfig, "thermostat algorithm is invalid"};
    }
    if (config.minSafeMilliCelsius >= config.maxSafeMilliCelsius) {
        return {DeviceError::InvalidConfig, "thermostat safe range is invalid"};
    }
    if (!(config.minSafeMilliCelsius < config.targetMilliCelsius && config.targetMilliCelsius < config.maxSafeMilliCelsius)) {
        return {DeviceError::InvalidConfig, "thermostat target must be inside safe range"};
    }
    if (config.hysteresisCentiCelsius == 0U || config.hysteresisCentiCelsius > kThermostatMaxHysteresisCentiCelsius) {
        return {DeviceError::InvalidConfig, "thermostat hysteresis is invalid"};
    }
    if (config.checkIntervalMs < kThermostatMinIntervalMs || config.checkIntervalMs > kThermostatMaxIntervalMs) {
        return {DeviceError::InvalidConfig, "thermostat check interval is invalid"};
    }
    if (config.sensorTimeoutMs < kThermostatMinIntervalMs || config.sensorTimeoutMs > kThermostatMaxIntervalMs) {
        return {DeviceError::InvalidConfig, "thermostat sensor timeout is invalid"};
    }
    if (config.retryAfterErrorMs < kThermostatMinIntervalMs || config.retryAfterErrorMs > kThermostatMaxIntervalMs) {
        return {DeviceError::InvalidConfig, "thermostat retry timeout is invalid"};
    }
    if (config.minSwitchIntervalMs > kThermostatMaxIntervalMs) {
        return {DeviceError::InvalidConfig, "thermostat minimum switch interval is invalid"};
    }
    return {};
}

bool parseThermostatDeviceConfigJson(const JsonObjectConst& input, ThermostatDeviceConfigV1& config, const char*& error) {
    ThermostatMode mode{};
    ThermostatAlgorithm algorithm{};
    if (!parseMode(input["mode"], mode, error)) {
        return false;
    }
    if (!parseAlgorithm(input["algorithm"], algorithm, error)) {
        return false;
    }
    config.mode = static_cast<uint8_t>(mode);
    config.algorithm = static_cast<uint8_t>(algorithm);

    if (!parseTemperatureField(input, "target_milli_celsius", "target_celsius", config.targetMilliCelsius, error) ||
        !parseTemperatureField(input, "min_safe_milli_celsius", "min_safe_celsius", config.minSafeMilliCelsius, error) ||
        !parseTemperatureField(input, "max_safe_milli_celsius", "max_safe_celsius", config.maxSafeMilliCelsius, error)) {
        return false;
    }

    if (!parseHysteresisField(input, config.hysteresisCentiCelsius, error)) {
        return false;
    }

    if (!parseDuration(input["check_interval_ms"], config.checkIntervalMs, error) ||
        !parseDuration(input["sensor_timeout_ms"], config.sensorTimeoutMs, error) ||
        !parseDuration(input["retry_after_error_ms"], config.retryAfterErrorMs, error) ||
        !parseDuration(input["min_switch_interval_ms"], config.minSwitchIntervalMs, error)) {
        return false;
    }

    return true;
}

void writeThermostatDeviceConfigJson(const ThermostatDeviceConfigV1& config, JsonObject output) {
    ThermostatMode mode{};
    ThermostatAlgorithm algorithm{};
    (void)thermostatModeFromByte(config.mode, mode);
    (void)thermostatAlgorithmFromByte(config.algorithm, algorithm);

    writeDeviceBaseConfigJson(config.base, output);
    output["mode"] = thermostatModeName(mode);
    output["algorithm"] = thermostatAlgorithmName(algorithm);
    output["target_celsius"] = static_cast<float>(config.targetMilliCelsius) / 1000.0F;
    output["target_milli_celsius"] = config.targetMilliCelsius;
    output["min_safe_celsius"] = static_cast<float>(config.minSafeMilliCelsius) / 1000.0F;
    output["min_safe_milli_celsius"] = config.minSafeMilliCelsius;
    output["max_safe_celsius"] = static_cast<float>(config.maxSafeMilliCelsius) / 1000.0F;
    output["max_safe_milli_celsius"] = config.maxSafeMilliCelsius;
    output["hysteresis_celsius"] = static_cast<float>(config.hysteresisCentiCelsius) / 100.0F;
    output["hysteresis_centi_celsius"] = config.hysteresisCentiCelsius;
    output["check_interval_ms"] = config.checkIntervalMs;
    output["sensor_timeout_ms"] = config.sensorTimeoutMs;
    output["retry_after_error_ms"] = config.retryAfterErrorMs;
    output["min_switch_interval_ms"] = config.minSwitchIntervalMs;
}

} // namespace ewfm
