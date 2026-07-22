#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorConfig.h"

#include "devices/core/ConfigCodec.h"
#include "devices/sensors/ds18b20/Ds18b20OneWireProtocol.h"

#include <type_traits>

namespace ewfm {

namespace {

bool parseBoolField(const JsonObjectConst& input, const char* key, bool defaultValue) {
    return (input[key] | defaultValue) ? true : false;
}

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

bool parseUint16(const JsonVariantConst& variant, uint16_t& value) {
    uint32_t parsed = value;
    if (!parseUint32(variant, parsed) || parsed > 65535UL) {
        return false;
    }
    value = static_cast<uint16_t>(parsed);
    return true;
}

bool parseReportDelta(const JsonObjectConst& input, uint16_t& centiCelsius) {
    const JsonVariantConst centiVariant = input["reportDeltaCentiCelsius"];
    if (!centiVariant.isNull()) {
        return parseUint16(centiVariant, centiCelsius);
    }

    const JsonVariantConst celsiusVariant = input["reportDeltaCelsius"];
    if (celsiusVariant.isNull()) {
        return true;
    }
    if (!celsiusVariant.is<float>() && !celsiusVariant.is<double>() && !celsiusVariant.is<int>()) {
        return false;
    }
    const float celsius = celsiusVariant.as<float>();
    if (celsius < 0.01F || celsius > 655.35F) {
        return false;
    }
    centiCelsius = static_cast<uint16_t>(celsius * 100.0F + 0.5F);
    return centiCelsius != 0U;
}
} // namespace

EWFM_LEGACY_CONFIG_USE_BEGIN
static_assert(std::is_trivially_copyable<Ds18b20TemperatureSensorConfigV1>::value, "Ds18b20TemperatureSensorConfigV1 must be POD");
static_assert(sizeof(Ds18b20TemperatureSensorConfigV1) == 51, "Ds18b20TemperatureSensorConfigV1 layout changed");
static_assert(sizeof(Ds18b20TemperatureSensorConfigV1::kMagic) - 1U + sizeof(Ds18b20TemperatureSensorConfigV1) <= kMaxDeviceConfigBytes,
              "Ds18b20TemperatureSensorConfigV1 exceeds device config bound");
EWFM_LEGACY_CONFIG_USE_END

static_assert(std::is_trivially_copyable<Ds18b20TemperatureSensorConfigV2>::value, "Ds18b20TemperatureSensorConfigV2 must be POD");
static_assert(sizeof(Ds18b20TemperatureSensorConfigV2) == 63, "Ds18b20TemperatureSensorConfigV2 layout changed");
static_assert(sizeof(Ds18b20TemperatureSensorConfigV2::kMagic) - 1U + sizeof(Ds18b20TemperatureSensorConfigV2) <= kMaxDeviceConfigBytes,
              "Ds18b20TemperatureSensorConfigV2 exceeds device config bound");

bool parseDs18b20TemperatureSensorConfigJson(const JsonObjectConst& input, Ds18b20TemperatureSensorConfigV2& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeDs18b20TemperatureSensorConfigJson(const Ds18b20TemperatureSensorConfigV2& config, JsonObject output) {
    config.writeJson(output);
}

EWFM_LEGACY_CONFIG_USE_BEGIN
DeviceValidationResult Ds18b20TemperatureSensorConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (!ds18b20AddressIsValid(address)) {
        return {DeviceError::InvalidConfig, "ds18b20 address is invalid"};
    }
    if (!ds18b20ResolutionIsValid(resolution)) {
        return {DeviceError::InvalidConfig, "ds18b20 resolution is invalid"};
    }
    TemperatureUnit unit{};
    if (!temperatureUnitFromByte(outputUnit, unit)) {
        return {DeviceError::InvalidConfig, "ds18b20 output unit is invalid"};
    }
    if (reportAlways > 1U) {
        return {DeviceError::InvalidConfig, "ds18b20 report policy is invalid"};
    }
    if (reportDeltaCentiCelsius == 0U) {
        return {DeviceError::InvalidConfig, "ds18b20 report delta is invalid"};
    }
    if (pollMs < kDs18b20MinPollMs || pollMs > kDs18b20MaxPollMs) {
        return {DeviceError::InvalidConfig, "ds18b20 poll period is invalid"};
    }
    return {};
}

EWFM_LEGACY_CONFIG_USE_END

DeviceValidationResult Ds18b20TemperatureSensorConfigV2::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (!ds18b20AddressIsValid(address)) {
        return {DeviceError::InvalidConfig, "ds18b20 address is invalid"};
    }
    if (!ds18b20ResolutionIsValid(resolution)) {
        return {DeviceError::InvalidConfig, "ds18b20 resolution is invalid"};
    }
    TemperatureUnit unit{};
    if (!temperatureUnitFromByte(outputUnit, unit)) {
        return {DeviceError::InvalidConfig, "ds18b20 output unit is invalid"};
    }
    if (reportAlways > 1U) {
        return {DeviceError::InvalidConfig, "ds18b20 report policy is invalid"};
    }
    if (reportDeltaCentiCelsius == 0U) {
        return {DeviceError::InvalidConfig, "ds18b20 report delta is invalid"};
    }
    if (pollMs < kDs18b20MinPollMs || pollMs > kDs18b20MaxPollMs) {
        return {DeviceError::InvalidConfig, "ds18b20 poll period is invalid"};
    }
    return filter.validate();
}

bool Ds18b20TemperatureSensorConfigV2::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    reportAlways = parseBoolField(input, "reportAlways", reportAlways != 0U) ? 1U : 0U;

    const JsonVariantConst addressVariant = input["address"];
    if (!addressVariant.isNull()) {
        if (!parseOneWireRomAddress(addressVariant.as<const char*>(), this->address)) {
            error = "ds18b20 address must be 16 hex characters";
            return false;
        }
    }

    const JsonVariantConst resolutionVariant = input["resolution"];
    if (!resolutionVariant.isNull()) {
        if (!resolutionVariant.is<int>()) {
            error = "ds18b20 resolution must be numeric";
            return false;
        }
        const int resolution = resolutionVariant.as<int>();
        if (resolution < 0 || resolution > 255) {
            error = "ds18b20 resolution is invalid";
            return false;
        }
        this->resolution = static_cast<uint8_t>(resolution);
    }

    TemperatureUnit unit{};
    if (!temperatureUnitFromString(input["unit"] | temperatureUnitName(static_cast<TemperatureUnit>(outputUnit)), unit)) {
        error = "ds18b20 output unit is invalid";
        return false;
    }
    outputUnit = temperatureUnitToByte(unit);

    uint32_t pollMs = this->pollMs;
    if (!parseUint32(input["pollMs"], pollMs)) {
        error = "ds18b20 poll period must be numeric";
        return false;
    }
    this->pollMs = pollMs;

    uint16_t reportDelta = this->reportDeltaCentiCelsius;
    if (!parseReportDelta(input, reportDelta)) {
        error = "ds18b20 report delta is invalid";
        return false;
    }
    reportDeltaCentiCelsius = reportDelta;

    if (!filter.parseJson(input, error)) {
        return false;
    }

    if (!ds18b20AddressIsValid(this->address)) {
        error = "ds18b20 address is invalid";
        return false;
    }
    if (!ds18b20ResolutionIsValid(resolution)) {
        error = "ds18b20 resolution is invalid";
        return false;
    }
    TemperatureUnit validatedUnit{};
    if (!temperatureUnitFromByte(outputUnit, validatedUnit)) {
        error = "ds18b20 output unit is invalid";
        return false;
    }
    if (reportAlways > 1U) {
        error = "ds18b20 report policy is invalid";
        return false;
    }
    if (reportDeltaCentiCelsius == 0U) {
        error = "ds18b20 report delta is invalid";
        return false;
    }
    if (pollMs < kDs18b20MinPollMs || pollMs > kDs18b20MaxPollMs) {
        error = "ds18b20 poll period is invalid";
        return false;
    }
    return true;
}

void Ds18b20TemperatureSensorConfigV2::writeJson(JsonObject output) const {
    char address[17]{};
    (void)formatOneWireRomAddress(this->address, address);

    DeviceBaseConfigV1::writeJson(output);
    output["address"] = address;
    output["resolution"] = resolution;
    output["unit"] = temperatureUnitName(static_cast<TemperatureUnit>(outputUnit));
    output["pollMs"] = pollMs;
    output["reportDeltaCelsius"] = static_cast<float>(reportDeltaCentiCelsius) / 100.0F;
    output["reportDeltaCentiCelsius"] = reportDeltaCentiCelsius;
    output["reportAlways"] = reportAlways != 0U;
    filter.writeJson(output);
}

EWFM_LEGACY_CONFIG_USE_BEGIN
void Ds18b20TemperatureSensorConfigV2::migrateFrom(const Ds18b20TemperatureSensorConfigV1& legacy) {
    static_cast<DeviceBaseConfigV1&>(*this) = static_cast<const DeviceBaseConfigV1&>(legacy);
    address = legacy.address;
    resolution = legacy.resolution;
    outputUnit = legacy.outputUnit;
    reportAlways = legacy.reportAlways;
    reportDeltaCentiCelsius = legacy.reportDeltaCentiCelsius;
    pollMs = legacy.pollMs;
    // filter left at pass-through defaults
}
EWFM_LEGACY_CONFIG_USE_END

bool decodeDs18b20TemperatureSensorConfig(const uint8_t* blob, size_t size, Ds18b20TemperatureSensorConfigV2& config) {
    if (decodeFixedConfigBlob(Ds18b20TemperatureSensorConfigV2::kMagic, blob, size, config) && config.validate().ok()) {
        return true;
    }
    EWFM_LEGACY_CONFIG_USE_BEGIN
    Ds18b20TemperatureSensorConfigV1 legacy{};
    if (!decodeFixedConfigBlob(Ds18b20TemperatureSensorConfigV1::kMagic, blob, size, legacy) || !legacy.validate().ok()) {
        return false;
    }
    EWFM_LEGACY_CONFIG_USE_END
    config.migrateFrom(legacy);
    return true;
}

} // namespace ewfm
