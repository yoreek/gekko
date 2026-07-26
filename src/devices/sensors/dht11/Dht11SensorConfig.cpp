#include "devices/sensors/dht11/Dht11SensorConfig.h"

#include "devices/core/ConfigCodec.h"
#include "devices/switch/gpio/GpioSwitchDeviceConfig.h"

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
        error = "dht11 filter must be an object";
        return false;
    }
    return filter.parseJson(variant.as<JsonObjectConst>(), error);
}

DeviceValidationResult validateDht11CommonConfig(const Dht11SensorConfigV1& config) {
    TemperatureUnit unit{};
    if (!temperatureUnitFromByte(config.outputUnit, unit)) {
        return {DeviceError::InvalidConfig, "dht11 output unit is invalid"};
    }
    if (config.reportAlways > 1U) {
        return {DeviceError::InvalidConfig, "dht11 report policy is invalid"};
    }
    if (config.reportDeltaCentiCelsius == 0U) {
        return {DeviceError::InvalidConfig, "dht11 temperature report delta is invalid"};
    }
    if (config.reportDeltaCentiPercent == 0U) {
        return {DeviceError::InvalidConfig, "dht11 humidity report delta is invalid"};
    }
    if (config.pollMs < kDht11MinPollMs || config.pollMs > kDht11MaxPollMs) {
        return {DeviceError::InvalidConfig, "dht11 poll period is invalid"};
    }
    const DeviceValidationResult temperatureFilterValidation = config.temperatureFilter.validate();
    if (!temperatureFilterValidation.ok()) {
        return temperatureFilterValidation;
    }
    return config.humidityFilter.validate();
}

bool parseDht11FieldsJson(const JsonObjectConst& input, Dht11SensorConfigV1& config, const char*& error) {
    uint32_t gpioPin = config.gpioPin;
    if (!parseUint32(input["gpioPin"], gpioPin) || gpioPin > 255U) {
        error = "dht11 gpio pin must be numeric";
        return false;
    }
    config.gpioPin = static_cast<uint8_t>(gpioPin);

    config.reportAlways = (input["reportAlways"] | (config.reportAlways != 0U)) ? 1U : 0U;

    TemperatureUnit unit{};
    if (!temperatureUnitFromString(input["unit"] | temperatureUnitName(static_cast<TemperatureUnit>(config.outputUnit)), unit)) {
        error = "dht11 output unit is invalid";
        return false;
    }
    config.outputUnit = temperatureUnitToByte(unit);

    uint32_t poll = config.pollMs;
    if (!parseUint32(input["pollMs"], poll)) {
        error = "dht11 poll period must be numeric";
        return false;
    }
    config.pollMs = poll;

    uint16_t temperatureDelta = config.reportDeltaCentiCelsius;
    if (!parseCentiDelta(input, "reportDeltaCentiCelsius", "reportDeltaCelsius", temperatureDelta)) {
        error = "dht11 temperature report delta must be numeric";
        return false;
    }
    config.reportDeltaCentiCelsius = temperatureDelta;

    uint16_t humidityDelta = config.reportDeltaCentiPercent;
    if (!parseCentiDelta(input, "reportDeltaCentiPercent", "reportDeltaHumidity", humidityDelta)) {
        error = "dht11 humidity report delta must be numeric";
        return false;
    }
    config.reportDeltaCentiPercent = humidityDelta;

    return parseFilterObject(input, "temperatureFilter", config.temperatureFilter, error) &&
           parseFilterObject(input, "humidityFilter", config.humidityFilter, error);
}

void writeDht11FieldsJson(const Dht11SensorConfigV1& config, JsonObject output) {
    output["gpioPin"] = config.gpioPin;
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

static_assert(std::is_trivially_copyable<Dht11SensorConfigV1>::value, "Dht11SensorConfigV1 must be POD");
static_assert(sizeof(Dht11SensorConfigV1::kMagic) - 1U + sizeof(Dht11SensorConfigV1) <= kMaxDeviceConfigBytes,
              "Dht11SensorConfigV1 exceeds device config bound");

bool decodeDht11SensorConfig(const uint8_t* blob, size_t size, Dht11SensorConfigV1& config) {
    return decodeFixedConfigBlob(Dht11SensorConfigV1::kMagic, blob, size, config) && config.validate().ok();
}

bool parseDht11SensorConfigJson(const JsonObjectConst& input, Dht11SensorConfigV1& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeDht11SensorConfigJson(const Dht11SensorConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

DeviceValidationResult Dht11SensorConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (!gpioSwitchPinIsValid(gpioPin)) {
        return {DeviceError::InvalidConfig, "dht11 gpio pin is invalid"};
    }
    return validateDht11CommonConfig(*this);
}

bool Dht11SensorConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error) || !parseDht11FieldsJson(input, *this, error)) {
        return false;
    }
    const DeviceValidationResult result = validate();
    if (!result.ok()) {
        error = result.message;
        return false;
    }
    return true;
}

void Dht11SensorConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    writeDht11FieldsJson(*this, output);
}

} // namespace ewfm
