#include "devices/sensors/aht10/Aht10SensorConfig.h"

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
        error = "aht10 filter must be an object";
        return false;
    }
    return filter.parseJson(variant.as<JsonObjectConst>(), error);
}

DeviceValidationResult validateAht10CommonConfig(const Aht10SensorConfigV1& config) {
    TemperatureUnit unit{};
    if (!temperatureUnitFromByte(config.outputUnit, unit)) {
        return {DeviceError::InvalidConfig, "aht10 output unit is invalid"};
    }
    if (config.reportAlways > 1U) {
        return {DeviceError::InvalidConfig, "aht10 report policy is invalid"};
    }
    if (config.reportDeltaCentiCelsius == 0U) {
        return {DeviceError::InvalidConfig, "aht10 temperature report delta is invalid"};
    }
    if (config.reportDeltaCentiPercent == 0U) {
        return {DeviceError::InvalidConfig, "aht10 humidity report delta is invalid"};
    }
    if (config.pollMs < kAht10MinPollMs || config.pollMs > kAht10MaxPollMs) {
        return {DeviceError::InvalidConfig, "aht10 poll period is invalid"};
    }
    const DeviceValidationResult temperatureFilterValidation = config.temperatureFilter.validate();
    if (!temperatureFilterValidation.ok()) {
        return temperatureFilterValidation;
    }
    return config.humidityFilter.validate();
}

bool parseAht10FieldsJson(const JsonObjectConst& input, Aht10SensorConfigV1& config, const char*& error) {
    config.reportAlways = (input["reportAlways"] | (config.reportAlways != 0U)) ? 1U : 0U;

    TemperatureUnit unit{};
    if (!temperatureUnitFromString(input["unit"] | temperatureUnitName(static_cast<TemperatureUnit>(config.outputUnit)), unit)) {
        error = "aht10 output unit is invalid";
        return false;
    }
    config.outputUnit = temperatureUnitToByte(unit);

    uint32_t poll = config.pollMs;
    if (!parseUint32(input["pollMs"], poll)) {
        error = "aht10 poll period must be numeric";
        return false;
    }
    config.pollMs = poll;

    uint16_t temperatureDelta = config.reportDeltaCentiCelsius;
    if (!parseCentiDelta(input, "reportDeltaCentiCelsius", "reportDeltaCelsius", temperatureDelta)) {
        error = "aht10 temperature report delta must be numeric";
        return false;
    }
    config.reportDeltaCentiCelsius = temperatureDelta;

    uint16_t humidityDelta = config.reportDeltaCentiPercent;
    if (!parseCentiDelta(input, "reportDeltaCentiPercent", "reportDeltaHumidity", humidityDelta)) {
        error = "aht10 humidity report delta must be numeric";
        return false;
    }
    config.reportDeltaCentiPercent = humidityDelta;

    return parseFilterObject(input, "temperatureFilter", config.temperatureFilter, error) &&
           parseFilterObject(input, "humidityFilter", config.humidityFilter, error);
}

void writeAht10FieldsJson(const Aht10SensorConfigV1& config, JsonObject output) {
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

static_assert(std::is_trivially_copyable<Aht10SensorConfigV1>::value, "Aht10SensorConfigV1 must be POD");
static_assert(std::is_base_of<I2cDeviceConfigV1, Aht10SensorConfigV1>::value, "Aht10SensorConfigV1 must use the shared I2C config");
static_assert(sizeof(Aht10SensorConfigV1::kMagic) - 1U + sizeof(Aht10SensorConfigV1) <= kMaxDeviceConfigBytes,
              "Aht10SensorConfigV1 exceeds device config bound");

bool decodeAht10SensorConfig(const uint8_t* blob, size_t size, Aht10SensorConfigV1& config) {
    return decodeFixedConfigBlob(Aht10SensorConfigV1::kMagic, blob, size, config) && config.validate().ok();
}

bool parseAht10SensorConfigJson(const JsonObjectConst& input, Aht10SensorConfigV1& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeAht10SensorConfigJson(const Aht10SensorConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

DeviceValidationResult Aht10SensorConfigV1::validate() const {
    const DeviceValidationResult i2cValidation = I2cDeviceConfigV1::validate();
    if (!i2cValidation.ok()) {
        return i2cValidation;
    }
    return validateAht10CommonConfig(*this);
}

bool Aht10SensorConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!I2cDeviceConfigV1::parseJson(input, error) || !parseAht10FieldsJson(input, *this, error)) {
        return false;
    }
    const DeviceValidationResult result = validate();
    if (!result.ok()) {
        error = result.message;
        return false;
    }
    return true;
}

void Aht10SensorConfigV1::writeJson(JsonObject output) const {
    I2cDeviceConfigV1::writeJson(output);
    writeAht10FieldsJson(*this, output);
}

} // namespace ewfm
