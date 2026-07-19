#include "devices/expander/PortExpanderConfig.h"

#include "devices/bus/i2c/I2cAddress.h"
#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<Pcf857xExpanderConfigV1>::value, "Pcf857xExpanderConfigV1 must be POD");
static_assert(sizeof(Pcf857xExpanderConfigV1::kMagic) - 1U + sizeof(Pcf857xExpanderConfigV1) <= kMaxDeviceConfigBytes,
              "Pcf857xExpanderConfigV1 exceeds device config bound");
static_assert(std::is_trivially_copyable<Pcf857xExpanderConfigV2>::value, "Pcf857xExpanderConfigV2 must be POD");
static_assert(std::is_base_of<I2cDeviceConfigV1, Pcf857xExpanderConfigV2>::value, "Pcf857xExpanderConfigV2 must use the shared I2C config");
static_assert(sizeof(Pcf857xExpanderConfigV2::kMagic) - 1U + sizeof(Pcf857xExpanderConfigV2) <= kMaxDeviceConfigBytes,
              "Pcf857xExpanderConfigV2 exceeds device config bound");

bool decodePcf857xExpanderConfig(const uint8_t* blob, const size_t size, Pcf857xExpanderConfigV2& config) {
    if (decodeFixedConfigBlob(Pcf857xExpanderConfigV2::kMagic, blob, size, config) && config.validate().ok()) {
        return true;
    }
    Pcf857xExpanderConfigV1 legacy{};
    if (!decodeFixedConfigBlob(Pcf857xExpanderConfigV1::kMagic, blob, size, legacy) || !legacy.validate().ok()) {
        return false;
    }
    config.migrateFrom(legacy);
    return config.validate().ok();
}

bool parsePcf857xExpanderConfigJson(const JsonObjectConst& input, Pcf857xExpanderConfigV1& config, const char*& error) {
    return config.parseJson(input, error);
}

void writePcf857xExpanderConfigJson(const Pcf857xExpanderConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

bool parsePcf857xExpanderConfigJson(const JsonObjectConst& input, Pcf857xExpanderConfigV2& config, const char*& error) {
    return config.parseJson(input, error);
}

void writePcf857xExpanderConfigJson(const Pcf857xExpanderConfigV2& config, JsonObject output) {
    config.writeJson(output);
}

DeviceValidationResult Pcf857xExpanderConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    const DeviceValidationResult addressValidation = validateI2cAddress(i2cAddress);
    if (!addressValidation.ok()) {
        return addressValidation;
    }
    if (inverted > 1U) {
        return {DeviceError::InvalidConfig, "port expander inverted flag is invalid"};
    }
    return {};
}

bool Pcf857xExpanderConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }
    inverted = (input["inverted"] | (inverted != 0U)) ? 1U : 0U;

    if (!parseI2cAddressJson(input["i2cAddress"], i2cAddress, error)) {
        return false;
    }
    return true;
}

void Pcf857xExpanderConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    writeI2cAddressJson(i2cAddress, output);
    output["inverted"] = inverted != 0U;
}

DeviceValidationResult Pcf857xExpanderConfigV2::validate() const {
    const DeviceValidationResult i2cValidation = I2cDeviceConfigV1::validate();
    if (!i2cValidation.ok()) {
        return i2cValidation;
    }
    if (inverted > 1U) {
        return {DeviceError::InvalidConfig, "port expander inverted flag is invalid"};
    }
    return {};
}

bool Pcf857xExpanderConfigV2::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!I2cDeviceConfigV1::parseJson(input, error)) {
        return false;
    }
    inverted = (input["inverted"] | (inverted != 0U)) ? 1U : 0U;
    return true;
}

void Pcf857xExpanderConfigV2::writeJson(JsonObject output) const {
    I2cDeviceConfigV1::writeJson(output);
    output["inverted"] = inverted != 0U;
}

void Pcf857xExpanderConfigV2::migrateFrom(const Pcf857xExpanderConfigV1& legacy) {
    enabled = legacy.enabled;
    std::memcpy(name, legacy.name, sizeof(name));
    i2cAddress = legacy.i2cAddress;
    inverted = legacy.inverted;
}

} // namespace ewfm
