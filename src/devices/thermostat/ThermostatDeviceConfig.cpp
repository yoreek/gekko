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

bool parseTemperatureField(const JsonObjectConst& input, const char* key, int32_t& milliCelsius, const char*& error) {
    const JsonVariantConst celsiusVariant = input[key];
    if (celsiusVariant.isNull()) {
        return true;
    }
    return parseTemperatureMilliCelsius(celsiusVariant, milliCelsius, error);
}

bool parseHysteresisField(const JsonObjectConst& input, uint16_t& centiCelsius, const char*& error) {
    const JsonVariantConst centiVariant = input["hysteresisCentiCelsius"];
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

    const JsonVariantConst celsiusVariant = input["hysteresisCelsius"];
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
    return config.validate();
}

bool parseThermostatDeviceConfigJson(const JsonObjectConst& input, ThermostatDeviceConfigV1& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeThermostatDeviceConfigJson(const ThermostatDeviceConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

bool ThermostatDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    ThermostatMode mode{};
    ThermostatAlgorithm algorithm{};
    (void)thermostatModeFromByte(this->mode, mode);
    (void)thermostatAlgorithmFromByte(this->algorithm, algorithm);
    if (!parseMode(input["mode"], mode, error)) {
        return false;
    }
    if (!parseAlgorithm(input["algorithm"], algorithm, error)) {
        return false;
    }
    this->mode = static_cast<uint8_t>(mode);
    this->algorithm = static_cast<uint8_t>(algorithm);

    if (!parseTemperatureField(input, "targetCelsius", targetMilliCelsius, error) ||
        !parseTemperatureField(input, "minSafeCelsius", minSafeMilliCelsius, error) ||
        !parseTemperatureField(input, "maxSafeCelsius", maxSafeMilliCelsius, error)) {
        return false;
    }

    if (!parseHysteresisField(input, hysteresisCentiCelsius, error)) {
        return false;
    }

    if (!parseDuration(input["checkIntervalMs"], checkIntervalMs, error) ||
        !parseDuration(input["sensorTimeoutMs"], sensorTimeoutMs, error) ||
        !parseDuration(input["retryAfterErrorMs"], retryAfterErrorMs, error) ||
        !parseDuration(input["minSwitchIntervalMs"], minSwitchIntervalMs, error)) {
        return false;
    }

    return true;
}

DeviceValidationResult ThermostatDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }

    ThermostatMode mode{};
    ThermostatAlgorithm algorithm{};
    if (!thermostatModeFromByte(this->mode, mode)) {
        return {DeviceError::InvalidConfig, "thermostat mode is invalid"};
    }
    if (!thermostatAlgorithmFromByte(this->algorithm, algorithm)) {
        return {DeviceError::InvalidConfig, "thermostat algorithm is invalid"};
    }
    if (minSafeMilliCelsius >= maxSafeMilliCelsius) {
        return {DeviceError::InvalidConfig, "thermostat safe range is invalid"};
    }
    if (!(minSafeMilliCelsius < targetMilliCelsius && targetMilliCelsius < maxSafeMilliCelsius)) {
        return {DeviceError::InvalidConfig, "thermostat target must be inside safe range"};
    }
    if (hysteresisCentiCelsius == 0U || hysteresisCentiCelsius > kThermostatMaxHysteresisCentiCelsius) {
        return {DeviceError::InvalidConfig, "thermostat hysteresis is invalid"};
    }
    if (checkIntervalMs < kThermostatMinIntervalMs || checkIntervalMs > kThermostatMaxIntervalMs) {
        return {DeviceError::InvalidConfig, "thermostat check interval is invalid"};
    }
    if (sensorTimeoutMs < kThermostatMinIntervalMs || sensorTimeoutMs > kThermostatMaxIntervalMs) {
        return {DeviceError::InvalidConfig, "thermostat sensor timeout is invalid"};
    }
    if (retryAfterErrorMs < kThermostatMinIntervalMs || retryAfterErrorMs > kThermostatMaxIntervalMs) {
        return {DeviceError::InvalidConfig, "thermostat retry timeout is invalid"};
    }
    if (minSwitchIntervalMs > kThermostatMaxIntervalMs) {
        return {DeviceError::InvalidConfig, "thermostat minimum switch interval is invalid"};
    }
    return {};
}

void ThermostatDeviceConfigV1::writeJson(JsonObject output) const {
    ThermostatMode mode{};
    ThermostatAlgorithm algorithm{};
    (void)thermostatModeFromByte(this->mode, mode);
    (void)thermostatAlgorithmFromByte(this->algorithm, algorithm);

    DeviceBaseConfigV1::writeJson(output);
    output["mode"] = thermostatModeName(mode);
    output["algorithm"] = thermostatAlgorithmName(algorithm);
    output["targetCelsius"] = static_cast<float>(targetMilliCelsius) / 1000.0F;
    output["targetMilliCelsius"] = targetMilliCelsius;
    output["minSafeCelsius"] = static_cast<float>(minSafeMilliCelsius) / 1000.0F;
    output["minSafeMilliCelsius"] = minSafeMilliCelsius;
    output["maxSafeCelsius"] = static_cast<float>(maxSafeMilliCelsius) / 1000.0F;
    output["maxSafeMilliCelsius"] = maxSafeMilliCelsius;
    output["hysteresisCelsius"] = static_cast<float>(hysteresisCentiCelsius) / 100.0F;
    output["hysteresisCentiCelsius"] = hysteresisCentiCelsius;
    output["checkIntervalMs"] = checkIntervalMs;
    output["sensorTimeoutMs"] = sensorTimeoutMs;
    output["retryAfterErrorMs"] = retryAfterErrorMs;
    output["minSwitchIntervalMs"] = minSwitchIntervalMs;
}

} // namespace ewfm
