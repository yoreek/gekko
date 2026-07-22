#include "devices/sensors/htu21/Htu21SensorConfig.h"

#include "devices/bus/i2c/I2cAddress.h"
#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {

bool parseUint32(const JsonVariantConst& variant, uint32_t& value) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<unsigned long>() && !variant.is<unsigned int>() && !variant.is<int>()) {
        return false;
    }
    const long parsed = variant.as<long>();
    if (parsed < 0) {
        return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool parseCentiDelta(const JsonObjectConst& input, const char* centiKey, const char* unitKey, uint16_t& centiValue) {
    const JsonVariantConst centiVariant = input[centiKey];
    if (!centiVariant.isNull()) {
        uint32_t parsed = centiValue;
        if (!parseUint32(centiVariant, parsed) || parsed > 65535UL) {
            return false;
        }
        centiValue = static_cast<uint16_t>(parsed);
        return true;
    }

    const JsonVariantConst unitVariant = input[unitKey];
    if (unitVariant.isNull()) {
        return true;
    }
    if (!unitVariant.is<float>() && !unitVariant.is<double>() && !unitVariant.is<int>()) {
        return false;
    }
    const float unitValue = unitVariant.as<float>();
    if (unitValue < 0.01F || unitValue > 655.35F) {
        return false;
    }
    centiValue = static_cast<uint16_t>(unitValue * 100.0F + 0.5F);
    return centiValue != 0U;
}

bool parseFilterObject(const JsonObjectConst& input, const char* key, SensorFilterConfigV1& filter, const char*& error) {
    const JsonVariantConst variant = input[key];
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<JsonObjectConst>()) {
        error = "htu21 filter must be an object";
        return false;
    }
    return filter.parseJson(variant.as<JsonObjectConst>(), error);
}

template <typename Config> DeviceValidationResult validateHtu21CommonConfig(const Config& config) {
    TemperatureUnit unit{};
    if (!temperatureUnitFromByte(config.outputUnit, unit)) {
        return {DeviceError::InvalidConfig, "htu21 output unit is invalid"};
    }
    if (config.reportAlways > 1U) {
        return {DeviceError::InvalidConfig, "htu21 report policy is invalid"};
    }
    if (config.reportDeltaCentiCelsius == 0U) {
        return {DeviceError::InvalidConfig, "htu21 temperature report delta is invalid"};
    }
    if (config.reportDeltaCentiPercent == 0U) {
        return {DeviceError::InvalidConfig, "htu21 humidity report delta is invalid"};
    }
    if (config.pollMs < kHtu21MinPollMs || config.pollMs > kHtu21MaxPollMs) {
        return {DeviceError::InvalidConfig, "htu21 poll period is invalid"};
    }
    const DeviceValidationResult temperatureFilterValidation = config.temperatureFilter.validate();
    if (!temperatureFilterValidation.ok()) {
        return temperatureFilterValidation;
    }
    return config.humidityFilter.validate();
}

template <typename Config> bool parseHtu21FieldsJson(const JsonObjectConst& input, Config& config, const char*& error) {
    config.reportAlways = (input["reportAlways"] | (config.reportAlways != 0U)) ? 1U : 0U;

    TemperatureUnit unit{};
    if (!temperatureUnitFromString(input["unit"] | temperatureUnitName(static_cast<TemperatureUnit>(config.outputUnit)), unit)) {
        error = "htu21 output unit is invalid";
        return false;
    }
    config.outputUnit = temperatureUnitToByte(unit);

    uint32_t poll = config.pollMs;
    if (!parseUint32(input["pollMs"], poll)) {
        error = "htu21 poll period must be numeric";
        return false;
    }
    config.pollMs = poll;

    uint16_t temperatureDelta = config.reportDeltaCentiCelsius;
    if (!parseCentiDelta(input, "reportDeltaCentiCelsius", "reportDeltaCelsius", temperatureDelta)) {
        error = "htu21 temperature report delta must be numeric";
        return false;
    }
    config.reportDeltaCentiCelsius = temperatureDelta;

    uint16_t humidityDelta = config.reportDeltaCentiPercent;
    if (!parseCentiDelta(input, "reportDeltaCentiPercent", "reportDeltaHumidity", humidityDelta)) {
        error = "htu21 humidity report delta must be numeric";
        return false;
    }
    config.reportDeltaCentiPercent = humidityDelta;

    return parseFilterObject(input, "temperatureFilter", config.temperatureFilter, error) &&
           parseFilterObject(input, "humidityFilter", config.humidityFilter, error);
}

template <typename Config> void writeHtu21FieldsJson(const Config& config, JsonObject output) {
    output["unit"] = temperatureUnitName(static_cast<TemperatureUnit>(config.outputUnit));
    output["pollMs"] = config.pollMs;
    output["reportDeltaCelsius"] = static_cast<float>(config.reportDeltaCentiCelsius) / 100.0F;
    output["reportDeltaCentiCelsius"] = config.reportDeltaCentiCelsius;
    output["reportDeltaHumidity"] = static_cast<float>(config.reportDeltaCentiPercent) / 100.0F;
    output["reportDeltaCentiPercent"] = config.reportDeltaCentiPercent;
    output["reportAlways"] = config.reportAlways != 0U;
    config.temperatureFilter.writeJson(output["temperatureFilter"].to<JsonObject>());
    config.humidityFilter.writeJson(output["humidityFilter"].to<JsonObject>());
}

} // namespace

EWFM_LEGACY_CONFIG_USE_BEGIN
static_assert(std::is_trivially_copyable<Htu21SensorConfigV1>::value, "Htu21SensorConfigV1 must be POD");
static_assert(sizeof(Htu21SensorConfigV1::kMagic) - 1U + sizeof(Htu21SensorConfigV1) <= kMaxDeviceConfigBytes,
              "Htu21SensorConfigV1 exceeds device config bound");
static_assert(std::is_trivially_copyable<Htu21SensorConfigV2>::value, "Htu21SensorConfigV2 must be POD");
static_assert(sizeof(Htu21SensorConfigV2::kMagic) - 1U + sizeof(Htu21SensorConfigV2) <= kMaxDeviceConfigBytes,
              "Htu21SensorConfigV2 exceeds device config bound");
EWFM_LEGACY_CONFIG_USE_END
static_assert(std::is_trivially_copyable<Htu21SensorConfigV3>::value, "Htu21SensorConfigV3 must be POD");
static_assert(std::is_base_of<I2cDeviceConfigV1, Htu21SensorConfigV3>::value, "Htu21SensorConfigV3 must use the shared I2C config");
static_assert(sizeof(Htu21SensorConfigV3::kMagic) - 1U + sizeof(Htu21SensorConfigV3) <= kMaxDeviceConfigBytes,
              "Htu21SensorConfigV3 exceeds device config bound");

bool decodeHtu21SensorConfig(const uint8_t* blob, size_t size, Htu21SensorConfigV3& config) {
    if (decodeFixedConfigBlob(Htu21SensorConfigV3::kMagic, blob, size, config) && config.validate().ok()) {
        return true;
    }
    EWFM_LEGACY_CONFIG_USE_BEGIN
    Htu21SensorConfigV2 legacyV2{};
    if (decodeFixedConfigBlob(Htu21SensorConfigV2::kMagic, blob, size, legacyV2) && legacyV2.validate().ok()) {
        config.migrateFrom(legacyV2);
        return config.validate().ok();
    }
    Htu21SensorConfigV1 legacy{};
    if (!decodeFixedConfigBlob(Htu21SensorConfigV1::kMagic, blob, size, legacy) || !legacy.validate().ok()) {
        return false;
    }
    config.migrateFrom(legacy);
    return config.validate().ok();
    EWFM_LEGACY_CONFIG_USE_END
}

bool parseHtu21SensorConfigJson(const JsonObjectConst& input, Htu21SensorConfigV3& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeHtu21SensorConfigJson(const Htu21SensorConfigV3& config, JsonObject output) {
    config.writeJson(output);
}

EWFM_LEGACY_CONFIG_USE_BEGIN
DeviceValidationResult Htu21SensorConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    return validateHtu21CommonConfig(*this);
}

DeviceValidationResult Htu21SensorConfigV2::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    const DeviceValidationResult commonValidation = validateHtu21CommonConfig(*this);
    if (!commonValidation.ok()) {
        return commonValidation;
    }
    return validateI2cAddress(i2cAddress);
}
EWFM_LEGACY_CONFIG_USE_END

DeviceValidationResult Htu21SensorConfigV3::validate() const {
    const DeviceValidationResult i2cValidation = I2cDeviceConfigV1::validate();
    if (!i2cValidation.ok()) {
        return i2cValidation;
    }
    return validateHtu21CommonConfig(*this);
}

bool Htu21SensorConfigV3::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!I2cDeviceConfigV1::parseJson(input, error) || !parseHtu21FieldsJson(input, *this, error)) {
        return false;
    }
    const DeviceValidationResult result = validate();
    if (!result.ok()) {
        error = result.message;
        return false;
    }
    return true;
}

void Htu21SensorConfigV3::writeJson(JsonObject output) const {
    I2cDeviceConfigV1::writeJson(output);
    writeHtu21FieldsJson(*this, output);
}

EWFM_LEGACY_CONFIG_USE_BEGIN
void Htu21SensorConfigV3::migrateFrom(const Htu21SensorConfigV1& legacy) {
    enabled = legacy.enabled;
    std::memcpy(name, legacy.name, sizeof(name));
    outputUnit = legacy.outputUnit;
    reportAlways = legacy.reportAlways;
    reportDeltaCentiCelsius = legacy.reportDeltaCentiCelsius;
    reportDeltaCentiPercent = legacy.reportDeltaCentiPercent;
    pollMs = legacy.pollMs;
    temperatureFilter = legacy.temperatureFilter;
    humidityFilter = legacy.humidityFilter;
    i2cAddress = kHtu21DefaultI2cAddress;
}

void Htu21SensorConfigV3::migrateFrom(const Htu21SensorConfigV2& legacy) {
    enabled = legacy.enabled;
    std::memcpy(name, legacy.name, sizeof(name));
    i2cAddress = legacy.i2cAddress;
    outputUnit = legacy.outputUnit;
    reportAlways = legacy.reportAlways;
    reportDeltaCentiCelsius = legacy.reportDeltaCentiCelsius;
    reportDeltaCentiPercent = legacy.reportDeltaCentiPercent;
    pollMs = legacy.pollMs;
    temperatureFilter = legacy.temperatureFilter;
    humidityFilter = legacy.humidityFilter;
}
EWFM_LEGACY_CONFIG_USE_END

} // namespace ewfm
