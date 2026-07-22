#include "devices/display/ssd1306/Ssd1306DeviceConfig.h"

#include "devices/bus/i2c/I2cAddress.h"
#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

EWFM_LEGACY_CONFIG_USE_BEGIN
static_assert(std::is_trivially_copyable<Ssd1306DeviceConfigV1>::value, "Ssd1306DeviceConfigV1 must be POD");
static_assert(sizeof(Ssd1306DeviceConfigV1::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV1) == ssd1306DeviceConfigV1Size(),
              "Ssd1306DeviceConfigV1 size mismatch");
static_assert(std::is_trivially_copyable<Ssd1306DeviceConfigV2>::value, "Ssd1306DeviceConfigV2 must be POD");
static_assert(sizeof(Ssd1306DeviceConfigV2::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV2) <= kMaxDeviceConfigBytes,
              "Ssd1306DeviceConfigV2 exceeds device config bound");
static_assert(std::is_trivially_copyable<Ssd1306DeviceConfigV3>::value, "Ssd1306DeviceConfigV3 must be POD");
static_assert(sizeof(Ssd1306DeviceConfigV3::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV3) <= kMaxDeviceConfigBytes,
              "Ssd1306DeviceConfigV3 exceeds device config bound");
static_assert(std::is_trivially_copyable<Ssd1306DeviceConfigV4>::value, "Ssd1306DeviceConfigV4 must be POD");
static_assert(sizeof(Ssd1306DeviceConfigV4::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV4) <= kMaxDeviceConfigBytes,
              "Ssd1306DeviceConfigV4 exceeds device config bound");
EWFM_LEGACY_CONFIG_USE_END
static_assert(std::is_trivially_copyable<Ssd1306DeviceConfigV5>::value, "Ssd1306DeviceConfigV5 must be POD");
static_assert(std::is_base_of<I2cDeviceConfigV1, Ssd1306DeviceConfigV5>::value, "Ssd1306DeviceConfigV5 must use the shared I2C config");
static_assert(sizeof(Ssd1306DeviceConfigV5::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV5) <= kMaxDeviceConfigBytes,
              "Ssd1306DeviceConfigV5 exceeds device config bound");

namespace {
template <typename Config> DeviceValidationResult validateSsd1306CommonConfig(const Config& config, uint16_t width, uint16_t height) {
    const DeviceValidationResult baseValidation = config.DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    const DeviceValidationResult addressValidation = validateI2cAddress(config.i2cAddress);
    if (!addressValidation.ok()) {
        return addressValidation;
    }
    if (width == 0U || height == 0U) {
        return {DeviceError::InvalidConfig, "ssd1306 layout dimensions must be positive"};
    }
    return {};
}

EWFM_LEGACY_CONFIG_USE_BEGIN
DeviceValidationResult validateSsd1306LegacyConfig(const Ssd1306DeviceConfigV1& config) {
    if (config.i2cBusDeviceId == 0U) {
        return {DeviceError::InvalidConfig, "ssd1306 i2c bus device id is required"};
    }
    return validateSsd1306CommonConfig(config, config.layoutWidth, config.layoutHeight);
}

DeviceValidationResult validateSsd1306LegacyConfig(const Ssd1306DeviceConfigV2& config) {
    if (config.i2cBusDeviceId == 0U) {
        return {DeviceError::InvalidConfig, "ssd1306 i2c bus device id is required"};
    }
    return validateSsd1306CommonConfig(config, config.width, config.height);
}

DeviceValidationResult validateSsd1306LegacyConfig(const Ssd1306DeviceConfigV3& config) {
    if (config.i2cBusDeviceId == 0U) {
        return {DeviceError::InvalidConfig, "ssd1306 i2c bus device id is required"};
    }
    const DeviceValidationResult commonValidation = validateSsd1306CommonConfig(config, config.width, config.height);
    if (!commonValidation.ok()) {
        return commonValidation;
    }
    if (config.rotation > 3U) {
        return {DeviceError::InvalidConfig, "ssd1306 rotation is out of bounds"};
    }
    return {};
}
EWFM_LEGACY_CONFIG_USE_END

template <typename Config> bool parseSsd1306FieldsJson(const JsonObjectConst& input, Config& config, const char*& error) {
    if (!input["layoutWidth"].isNull() || !input["layoutHeight"].isNull()) {
        error = "ssd1306 layout dimensions must use width and height";
        return false;
    }

    const JsonVariantConst rotationVariant = input["rotation"];
    if (!rotationVariant.isNull()) {
        if (!rotationVariant.is<unsigned long>() && !rotationVariant.is<long>() && !rotationVariant.is<int>()) {
            error = "ssd1306 rotation must be numeric";
            return false;
        }
        const long parsed = rotationVariant.as<long>();
        if (parsed < 0 || parsed > 3) {
            error = "ssd1306 rotation is out of bounds";
            return false;
        }
        config.rotation = static_cast<uint8_t>(parsed);
    }

    const JsonVariantConst widthVariant = input["width"];
    if (!widthVariant.isNull()) {
        const long parsed = widthVariant.as<long>();
        if (parsed <= 0 || parsed > 255) {
            error = "ssd1306 layout width is out of bounds";
            return false;
        }
        config.width = static_cast<uint16_t>(parsed);
    }

    const JsonVariantConst heightVariant = input["height"];
    if (!heightVariant.isNull()) {
        const long parsed = heightVariant.as<long>();
        if (parsed <= 0 || parsed > 255) {
            error = "ssd1306 layout height is out of bounds";
            return false;
        }
        config.height = static_cast<uint16_t>(parsed);
    }
    return true;
}
} // namespace

bool decodeSsd1306DeviceConfig(const uint8_t* blob, size_t size, Ssd1306DeviceConfigV5& config) {
    if (decodeFixedConfigBlob(Ssd1306DeviceConfigV5::kMagic, blob, size, config) && config.validate().ok()) {
        return true;
    }
    EWFM_LEGACY_CONFIG_USE_BEGIN
    Ssd1306DeviceConfigV4 legacyV4{};
    if (decodeFixedConfigBlob(Ssd1306DeviceConfigV4::kMagic, blob, size, legacyV4) && legacyV4.validate().ok()) {
        config.migrateFrom(legacyV4);
        return config.validate().ok();
    }
    Ssd1306DeviceConfigV3 legacyV3{};
    if (decodeFixedConfigBlob(Ssd1306DeviceConfigV3::kMagic, blob, size, legacyV3) && validateSsd1306LegacyConfig(legacyV3).ok()) {
        config.migrateFrom(legacyV3);
        return config.validate().ok();
    }
    Ssd1306DeviceConfigV2 legacyV2{};
    if (decodeFixedConfigBlob(Ssd1306DeviceConfigV2::kMagic, blob, size, legacyV2) && validateSsd1306LegacyConfig(legacyV2).ok()) {
        config.migrateFrom(legacyV2);
        return config.validate().ok();
    }
    Ssd1306DeviceConfigV1 legacy{};
    if (!decodeFixedConfigBlob(Ssd1306DeviceConfigV1::kMagic, blob, size, legacy) || !validateSsd1306LegacyConfig(legacy).ok()) {
        return false;
    }
    config.migrateFrom(legacy);
    return config.validate().ok();
    EWFM_LEGACY_CONFIG_USE_END
}

EWFM_LEGACY_CONFIG_USE_BEGIN
void Ssd1306DeviceConfigV4::migrateFrom(const Ssd1306DeviceConfigV1& origState) {
    enabled = origState.enabled;
    std::memcpy(name, origState.name, sizeof(name));
    i2cAddress = origState.i2cAddress;
    rotation = 0U;
    width = origState.layoutWidth;
    height = origState.layoutHeight;
}

void Ssd1306DeviceConfigV4::migrateFrom(const Ssd1306DeviceConfigV2& origState) {
    enabled = origState.enabled;
    std::memcpy(name, origState.name, sizeof(name));
    i2cAddress = origState.i2cAddress;
    rotation = 0U;
    width = origState.width;
    height = origState.height;
}

void Ssd1306DeviceConfigV4::migrateFrom(const Ssd1306DeviceConfigV3& origState) {
    enabled = origState.enabled;
    std::memcpy(name, origState.name, sizeof(name));
    i2cAddress = origState.i2cAddress;
    rotation = origState.rotation;
    width = origState.width;
    height = origState.height;
}

DeviceValidationResult Ssd1306DeviceConfigV4::validate() const {
    const DeviceValidationResult commonValidation = validateSsd1306CommonConfig(*this, width, height);
    if (!commonValidation.ok()) {
        return commonValidation;
    }
    if (rotation > 3U) {
        return {DeviceError::InvalidConfig, "ssd1306 rotation is out of bounds"};
    }
    return {};
}

void Ssd1306DeviceConfigV5::migrateFrom(const Ssd1306DeviceConfigV1& origState) {
    Ssd1306DeviceConfigV4 intermediate{};
    intermediate.migrateFrom(origState);
    migrateFrom(intermediate);
}

void Ssd1306DeviceConfigV5::migrateFrom(const Ssd1306DeviceConfigV2& origState) {
    Ssd1306DeviceConfigV4 intermediate{};
    intermediate.migrateFrom(origState);
    migrateFrom(intermediate);
}

void Ssd1306DeviceConfigV5::migrateFrom(const Ssd1306DeviceConfigV3& origState) {
    Ssd1306DeviceConfigV4 intermediate{};
    intermediate.migrateFrom(origState);
    migrateFrom(intermediate);
}

void Ssd1306DeviceConfigV5::migrateFrom(const Ssd1306DeviceConfigV4& origState) {
    enabled = origState.enabled;
    std::memcpy(name, origState.name, sizeof(name));
    i2cAddress = origState.i2cAddress;
    rotation = origState.rotation;
    width = origState.width;
    height = origState.height;
}
EWFM_LEGACY_CONFIG_USE_END

bool Ssd1306DeviceConfigV5::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!input["i2cBusDeviceId"].isNull()) {
        error = "ssd1306 i2cBusDeviceId must be provided through deps";
        return false;
    }
    if (!I2cDeviceConfigV1::parseJson(input, error)) {
        return false;
    }
    return parseSsd1306FieldsJson(input, *this, error);
}

DeviceValidationResult Ssd1306DeviceConfigV5::validate() const {
    const DeviceValidationResult i2cValidation = I2cDeviceConfigV1::validate();
    if (!i2cValidation.ok()) {
        return i2cValidation;
    }
    if (width == 0U || height == 0U) {
        return {DeviceError::InvalidConfig, "ssd1306 layout dimensions must be positive"};
    }
    if (rotation > 3U) {
        return {DeviceError::InvalidConfig, "ssd1306 rotation is out of bounds"};
    }
    return {};
}

void Ssd1306DeviceConfigV5::writeJson(JsonObject output) const {
    I2cDeviceConfigV1::writeJson(output);
    output["rotation"] = rotation;
    output["width"] = width;
    output["height"] = height;
}

} // namespace ewfm
