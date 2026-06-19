#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorConfig.h"

#include "devices/sensors/ds18b20/Ds18b20OneWireProtocol.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
template <typename T> std::string encodeConfigBlob(uint32_t magicKey, const T& config) {
    std::string blob;
    blob.resize(sizeof(magicKey) + sizeof(T));
    std::memcpy(blob.data(), &magicKey, sizeof(magicKey));
    std::memcpy(blob.data() + sizeof(magicKey), &config, sizeof(T));
    return blob;
}

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
    const JsonVariantConst centiVariant = input["report_delta_centi_celsius"];
    if (!centiVariant.isNull()) {
        return parseUint16(centiVariant, centiCelsius);
    }

    const JsonVariantConst celsiusVariant = input["report_delta_celsius"];
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

static_assert(std::is_trivially_copyable<Ds18b20TemperatureSensorConfigV1>::value, "Ds18b20TemperatureSensorConfigV1 must be POD");
static_assert(sizeof(Ds18b20TemperatureSensorConfigV1) == 18, "Ds18b20TemperatureSensorConfigV1 layout changed");
static_assert(sizeof(Ds18b20TemperatureSensorConfigV1::kMagicKey) + sizeof(Ds18b20TemperatureSensorConfigV1) <= kMaxDeviceConfigBytes,
              "Ds18b20TemperatureSensorConfigV1 exceeds device config bound");

std::string encodeDs18b20TemperatureSensorConfig(const Ds18b20TemperatureSensorConfigV1& config) {
    return encodeConfigBlob(Ds18b20TemperatureSensorConfigV1::kMagicKey, config);
}

bool decodeDs18b20TemperatureSensorConfig(const std::string& blob, Ds18b20TemperatureSensorConfigV1& config) {
    constexpr size_t kBlobSize = sizeof(Ds18b20TemperatureSensorConfigV1::kMagicKey) + sizeof(Ds18b20TemperatureSensorConfigV1);
    if (blob.size() != kBlobSize) {
        return false;
    }

    uint32_t magicKey{0};
    std::memcpy(&magicKey, blob.data(), sizeof(magicKey));
    if (magicKey != Ds18b20TemperatureSensorConfigV1::kMagicKey) {
        return false;
    }

    std::memcpy(&config, blob.data() + sizeof(magicKey), sizeof(Ds18b20TemperatureSensorConfigV1));
    return validateDs18b20TemperatureSensorConfig(config).ok();
}

DeviceValidationResult validateDs18b20TemperatureSensorConfig(const Ds18b20TemperatureSensorConfigV1& config) {
    if (!ds18b20AddressIsValid(config.address)) {
        return {DeviceError::InvalidConfig, "ds18b20 address is invalid"};
    }
    if (!ds18b20ResolutionIsValid(config.resolution)) {
        return {DeviceError::InvalidConfig, "ds18b20 resolution is invalid"};
    }
    TemperatureUnit unit{};
    if (!temperatureUnitFromByte(config.outputUnit, unit)) {
        return {DeviceError::InvalidConfig, "ds18b20 output unit is invalid"};
    }
    if (config.reportAlways > 1U) {
        return {DeviceError::InvalidConfig, "ds18b20 report policy is invalid"};
    }
    if (config.reportDeltaCentiCelsius == 0U) {
        return {DeviceError::InvalidConfig, "ds18b20 report delta is invalid"};
    }
    if (config.pollMs < kDs18b20MinPollMs || config.pollMs > kDs18b20MaxPollMs) {
        return {DeviceError::InvalidConfig, "ds18b20 poll period is invalid"};
    }
    return {};
}

bool parseDs18b20TemperatureSensorConfigJson(const JsonObjectConst& input, Ds18b20TemperatureSensorConfigV1& config, std::string& error) {
    config.enabled = parseBoolField(input, "enabled", true) ? 1U : 0U;
    config.reportAlways = parseBoolField(input, "report_always", false) ? 1U : 0U;

    const char* address = input["address"] | "";
    if (!parseOneWireRomAddress(address, config.address)) {
        error = "ds18b20 address must be 16 hex characters";
        return false;
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
        config.resolution = static_cast<uint8_t>(resolution);
    }

    TemperatureUnit unit{};
    if (!temperatureUnitFromString(input["unit"] | "celsius", unit)) {
        error = "ds18b20 output unit is invalid";
        return false;
    }
    config.outputUnit = temperatureUnitToByte(unit);

    uint32_t pollMs = config.pollMs;
    if (!parseUint32(input["poll_ms"], pollMs)) {
        error = "ds18b20 poll period must be numeric";
        return false;
    }
    config.pollMs = pollMs;

    uint16_t reportDelta = config.reportDeltaCentiCelsius;
    if (!parseReportDelta(input, reportDelta)) {
        error = "ds18b20 report delta is invalid";
        return false;
    }
    config.reportDeltaCentiCelsius = reportDelta;

    const DeviceValidationResult validation = validateDs18b20TemperatureSensorConfig(config);
    if (!validation.ok()) {
        error = validation.message;
        return false;
    }
    return true;
}

void writeDs18b20TemperatureSensorConfigJson(const Ds18b20TemperatureSensorConfigV1& config, JsonObject output) {
    char address[17]{};
    (void)formatOneWireRomAddress(config.address, address);
    TemperatureUnit unit{TemperatureUnit::Celsius};
    (void)temperatureUnitFromByte(config.outputUnit, unit);

    output["enabled"] = config.enabled != 0U;
    output["address"] = address;
    output["resolution"] = config.resolution;
    output["unit"] = temperatureUnitName(unit);
    output["poll_ms"] = config.pollMs;
    output["report_delta_celsius"] = static_cast<float>(config.reportDeltaCentiCelsius) / 100.0F;
    output["report_delta_centi_celsius"] = config.reportDeltaCentiCelsius;
    output["report_always"] = config.reportAlways != 0U;
}

} // namespace ewfm
