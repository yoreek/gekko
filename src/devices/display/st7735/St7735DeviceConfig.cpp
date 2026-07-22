#include "devices/display/st7735/St7735DeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
EWFM_LEGACY_CONFIG_USE_BEGIN
DeviceValidationResult validateSt7735DeviceConfigV1(const St7735DeviceConfigV1& config) {
    const DeviceValidationResult baseValidation = config.DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (config.spiBusDeviceId == 0U) {
        return {DeviceError::InvalidConfig, "st7735 spi bus device id is required"};
    }
    if (config.layoutWidth == 0U || config.layoutHeight == 0U) {
        return {DeviceError::InvalidConfig, "st7735 layout dimensions must be positive"};
    }
    return {};
}

DeviceValidationResult validateSt7735DeviceConfigV2(const St7735DeviceConfigV2& config) {
    const DeviceValidationResult baseValidation = config.DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (config.spiBusDeviceId == 0U) {
        return {DeviceError::InvalidConfig, "st7735 spi bus device id is required"};
    }
    if (config.layoutWidth == 0U || config.layoutHeight == 0U) {
        return {DeviceError::InvalidConfig, "st7735 layout dimensions must be positive"};
    }
    return {};
}

DeviceValidationResult validateSt7735DeviceConfigV3(const St7735DeviceConfigV3& config) {
    const DeviceValidationResult baseValidation = config.DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (config.spiBusDeviceId == 0U) {
        return {DeviceError::InvalidConfig, "st7735 spi bus device id is required"};
    }
    if (config.width == 0U || config.height == 0U) {
        return {DeviceError::InvalidConfig, "st7735 layout dimensions must be positive"};
    }
    return {};
}
EWFM_LEGACY_CONFIG_USE_END
} // namespace

EWFM_LEGACY_CONFIG_USE_BEGIN
static_assert(std::is_trivially_copyable<St7735DeviceConfigV1>::value, "St7735DeviceConfigV1 must be POD");
static_assert(sizeof(St7735DeviceConfigV1::kMagic) - 1U + sizeof(St7735DeviceConfigV1) == st7735DeviceConfigV1Size(),
              "St7735DeviceConfigV1 size mismatch");
static_assert(std::is_trivially_copyable<St7735DeviceConfigV2>::value, "St7735DeviceConfigV2 must be POD");
static_assert(sizeof(St7735DeviceConfigV2::kMagic) - 1U + sizeof(St7735DeviceConfigV2) == st7735DeviceConfigV2Size(),
              "St7735DeviceConfigV2 size mismatch");
static_assert(std::is_trivially_copyable<St7735DeviceConfigV3>::value, "St7735DeviceConfigV3 must be POD");
static_assert(sizeof(St7735DeviceConfigV3::kMagic) - 1U + sizeof(St7735DeviceConfigV3) <= kMaxDeviceConfigBytes,
              "St7735DeviceConfigV3 exceeds device config bound");
EWFM_LEGACY_CONFIG_USE_END
static_assert(std::is_trivially_copyable<St7735DeviceConfigV4>::value, "St7735DeviceConfigV4 must be POD");
static_assert(sizeof(St7735DeviceConfigV4::kMagic) - 1U + sizeof(St7735DeviceConfigV4) <= kMaxDeviceConfigBytes,
              "St7735DeviceConfigV4 exceeds device config bound");

bool decodeSt7735DeviceConfig(const uint8_t* blob, size_t size, St7735DeviceConfigV4& config) {
    if (decodeFixedConfigBlob(St7735DeviceConfigV4::kMagic, blob, size, config) && config.validate().ok()) {
        return true;
    }
    EWFM_LEGACY_CONFIG_USE_BEGIN
    St7735DeviceConfigV3 legacyV3{};
    if (decodeFixedConfigBlob(St7735DeviceConfigV3::kMagic, blob, size, legacyV3) && validateSt7735DeviceConfigV3(legacyV3).ok()) {
        config.migrateFrom(legacyV3);
        return config.validate().ok();
    }
    St7735DeviceConfigV2 v2{};
    if (decodeFixedConfigBlob(St7735DeviceConfigV2::kMagic, blob, size, v2) && validateSt7735DeviceConfigV2(v2).ok()) {
        config.migrateFrom(v2);
        return config.validate().ok();
    }
    St7735DeviceConfigV1 v1{};
    if (decodeFixedConfigBlob(St7735DeviceConfigV1::kMagic, blob, size, v1) && validateSt7735DeviceConfigV1(v1).ok()) {
        config.migrateFrom(v1);
        return config.validate().ok();
    }
    EWFM_LEGACY_CONFIG_USE_END
    return false;
}

EWFM_LEGACY_CONFIG_USE_BEGIN
void St7735DeviceConfigV4::migrateFrom(const St7735DeviceConfigV1& origState) {
    enabled = origState.enabled;
    std::memcpy(name, origState.name, sizeof(name));
    spiBusDeviceId = origState.spiBusDeviceId;
    chipSelectPin = origState.chipSelectPin;
    dcPin = 2U;
    resetPin = -1;
    rotation = 0U;
    width = origState.layoutWidth;
    height = origState.layoutHeight;
}

void St7735DeviceConfigV4::migrateFrom(const St7735DeviceConfigV2& origState) {
    enabled = origState.enabled;
    std::memcpy(name, origState.name, sizeof(name));
    spiBusDeviceId = origState.spiBusDeviceId;
    chipSelectPin = origState.chipSelectPin;
    dcPin = origState.dcPin;
    resetPin = origState.resetPin;
    rotation = 0U;
    width = origState.layoutWidth;
    height = origState.layoutHeight;
}

void St7735DeviceConfigV4::migrateFrom(const St7735DeviceConfigV3& origState) {
    enabled = origState.enabled;
    std::memcpy(name, origState.name, sizeof(name));
    spiBusDeviceId = origState.spiBusDeviceId;
    chipSelectPin = origState.chipSelectPin;
    dcPin = origState.dcPin;
    resetPin = origState.resetPin;
    rotation = 0U;
    width = origState.width;
    height = origState.height;
}
EWFM_LEGACY_CONFIG_USE_END

bool St7735DeviceConfigV4::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    const JsonVariantConst busDeviceId = input["spiBusDeviceId"];
    if (!busDeviceId.isNull()) {
        if (!busDeviceId.is<unsigned long>() && !busDeviceId.is<long>() && !busDeviceId.is<int>()) {
            error = "st7735 spi bus device id must be numeric";
            return false;
        }
        const unsigned long parsed = busDeviceId.as<unsigned long>();
        if (parsed > 0xFFFFFFFFUL) {
            error = "st7735 spi bus device id is out of bounds";
            return false;
        }
        spiBusDeviceId = static_cast<uint32_t>(parsed);
    }

    const JsonVariantConst csVariant = input["chipSelectPin"];
    if (!csVariant.isNull()) {
        if (!csVariant.is<unsigned long>() && !csVariant.is<long>() && !csVariant.is<int>()) {
            error = "st7735 chip select pin must be numeric";
            return false;
        }
        const long parsed = csVariant.as<long>();
        if (parsed < 0 || parsed > 0xFF) {
            error = "st7735 chip select pin is out of bounds";
            return false;
        }
        chipSelectPin = static_cast<uint8_t>(parsed);
    }

    const JsonVariantConst dcVariant = input["dcPin"];
    if (!dcVariant.isNull()) {
        if (!dcVariant.is<unsigned long>() && !dcVariant.is<long>() && !dcVariant.is<int>()) {
            error = "st7735 dc pin must be numeric";
            return false;
        }
        const long parsed = dcVariant.as<long>();
        if (parsed < 0 || parsed > 0xFF) {
            error = "st7735 dc pin is out of bounds";
            return false;
        }
        dcPin = static_cast<uint8_t>(parsed);
    }

    const JsonVariantConst resetVariant = input["resetPin"];
    if (!resetVariant.isNull()) {
        if (!resetVariant.is<long>() && !resetVariant.is<int>() && !resetVariant.is<unsigned long>()) {
            error = "st7735 reset pin must be numeric";
            return false;
        }
        const long parsed = resetVariant.as<long>();
        if (parsed < -1 || parsed > 0x7F) {
            error = "st7735 reset pin is out of bounds";
            return false;
        }
        resetPin = static_cast<int8_t>(parsed);
    }

    if (!input["layoutWidth"].isNull() || !input["layoutHeight"].isNull()) {
        error = "st7735 layout dimensions must use width and height";
        return false;
    }

    const JsonVariantConst rotationVariant = input["rotation"];
    if (!rotationVariant.isNull()) {
        if (!rotationVariant.is<unsigned long>() && !rotationVariant.is<long>() && !rotationVariant.is<int>()) {
            error = "st7735 rotation must be numeric";
            return false;
        }
        const long parsed = rotationVariant.as<long>();
        if (parsed < 0 || parsed > 3) {
            error = "st7735 rotation is out of bounds";
            return false;
        }
        rotation = static_cast<uint8_t>(parsed);
    }

    const JsonVariantConst widthVariant = input["width"];
    if (!widthVariant.isNull()) {
        const long parsed = widthVariant.as<long>();
        if (parsed <= 0 || parsed > 255) {
            error = "st7735 layout width is out of bounds";
            return false;
        }
        width = static_cast<uint16_t>(parsed);
    }

    const JsonVariantConst heightVariant = input["height"];
    if (!heightVariant.isNull()) {
        const long parsed = heightVariant.as<long>();
        if (parsed <= 0 || parsed > 255) {
            error = "st7735 layout height is out of bounds";
            return false;
        }
        height = static_cast<uint16_t>(parsed);
    }

    return true;
}

DeviceValidationResult St7735DeviceConfigV4::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (spiBusDeviceId == 0U) {
        return {DeviceError::InvalidConfig, "st7735 spi bus device id is required"};
    }
    if (rotation > 3U) {
        return {DeviceError::InvalidConfig, "st7735 rotation is out of bounds"};
    }
    if (width == 0U || height == 0U) {
        return {DeviceError::InvalidConfig, "st7735 layout dimensions must be positive"};
    }
    return {};
}

void St7735DeviceConfigV4::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["spiBusDeviceId"] = spiBusDeviceId;
    output["chipSelectPin"] = chipSelectPin;
    output["dcPin"] = dcPin;
    output["resetPin"] = resetPin;
    output["rotation"] = rotation;
    output["width"] = width;
    output["height"] = height;
}

} // namespace ewfm
