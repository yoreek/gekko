#include "devices/display/ssd1306/Ssd1306DeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<Ssd1306DeviceConfigV1>::value, "Ssd1306DeviceConfigV1 must be POD");
static_assert(sizeof(Ssd1306DeviceConfigV1::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV1) == ssd1306DeviceConfigV1Size(),
              "Ssd1306DeviceConfigV1 size mismatch");
static_assert(std::is_trivially_copyable<Ssd1306DeviceConfigV2>::value, "Ssd1306DeviceConfigV2 must be POD");
static_assert(sizeof(Ssd1306DeviceConfigV2::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV2) <= kMaxDeviceConfigBytes,
              "Ssd1306DeviceConfigV2 exceeds device config bound");

namespace {
DeviceValidationResult validateSsd1306LegacyConfig(const Ssd1306DeviceConfigV1& config) {
    const DeviceValidationResult baseValidation = config.DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (config.i2cBusDeviceId == 0U) {
        return {DeviceError::InvalidConfig, "ssd1306 i2c bus device id is required"};
    }
    if (config.i2cAddress > 0x7FU) {
        return {DeviceError::InvalidConfig, "ssd1306 i2c address exceeds supported range"};
    }
    if (config.layoutWidth == 0U || config.layoutHeight == 0U) {
        return {DeviceError::InvalidConfig, "ssd1306 layout dimensions must be positive"};
    }
    return {};
}
} // namespace

bool encodeSsd1306DeviceConfig(const Ssd1306DeviceConfigV2& config, uint8_t* blob, size_t capacity) {
    return encodeFixedConfigBlob(Ssd1306DeviceConfigV2::kMagic, config, blob, capacity);
}

bool decodeSsd1306DeviceConfig(const uint8_t* blob, size_t size, Ssd1306DeviceConfigV2& config) {
    if (decodeFixedConfigBlob(Ssd1306DeviceConfigV2::kMagic, blob, size, config) && config.validate().ok()) {
        return true;
    }
    Ssd1306DeviceConfigV1 legacy{};
    if (!decodeFixedConfigBlob(Ssd1306DeviceConfigV1::kMagic, blob, size, legacy) || !validateSsd1306LegacyConfig(legacy).ok()) {
        return false;
    }
    config.migrateFrom(legacy);
    return config.validate().ok();
}

void Ssd1306DeviceConfigV2::migrateFrom(const Ssd1306DeviceConfigV1& origState) {
    enabled = origState.enabled;
    std::memcpy(name, origState.name, sizeof(name));
    i2cBusDeviceId = origState.i2cBusDeviceId;
    i2cAddress = origState.i2cAddress;
    width = origState.layoutWidth;
    height = origState.layoutHeight;
}

bool Ssd1306DeviceConfigV2::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    const JsonVariantConst busDeviceId = input["i2cBusDeviceId"];
    if (!busDeviceId.isNull()) {
        if (!busDeviceId.is<unsigned long>() && !busDeviceId.is<long>() && !busDeviceId.is<int>()) {
            error = "ssd1306 i2c bus device id must be numeric";
            return false;
        }
        const unsigned long parsed = busDeviceId.as<unsigned long>();
        if (parsed > 0xFFFFFFFFUL) {
            error = "ssd1306 i2c bus device id is out of bounds";
            return false;
        }
        i2cBusDeviceId = static_cast<uint32_t>(parsed);
    }

    const JsonVariantConst addressVariant = input["i2cAddress"];
    if (!addressVariant.isNull()) {
        if (!addressVariant.is<unsigned long>() && !addressVariant.is<long>() && !addressVariant.is<int>()) {
            error = "ssd1306 i2c address must be numeric";
            return false;
        }
        const long parsed = addressVariant.as<long>();
        if (parsed < 0 || parsed > 0x7F) {
            error = "ssd1306 i2c address is out of bounds";
            return false;
        }
        i2cAddress = static_cast<uint8_t>(parsed);
    }

    if (!input["layoutWidth"].isNull() || !input["layoutHeight"].isNull()) {
        error = "ssd1306 layout dimensions must use width and height";
        return false;
    }

    const JsonVariantConst widthVariant = input["width"];
    if (!widthVariant.isNull()) {
        const long parsed = widthVariant.as<long>();
        if (parsed <= 0 || parsed > 255) {
            error = "ssd1306 layout width is out of bounds";
            return false;
        }
        width = static_cast<uint16_t>(parsed);
    }

    const JsonVariantConst heightVariant = input["height"];
    if (!heightVariant.isNull()) {
        const long parsed = heightVariant.as<long>();
        if (parsed <= 0 || parsed > 255) {
            error = "ssd1306 layout height is out of bounds";
            return false;
        }
        height = static_cast<uint16_t>(parsed);
    }

    return true;
}

DeviceValidationResult Ssd1306DeviceConfigV2::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (i2cBusDeviceId == 0U) {
        return {DeviceError::InvalidConfig, "ssd1306 i2c bus device id is required"};
    }
    if (i2cAddress > 0x7FU) {
        return {DeviceError::InvalidConfig, "ssd1306 i2c address exceeds supported range"};
    }
    if (width == 0U || height == 0U) {
        return {DeviceError::InvalidConfig, "ssd1306 layout dimensions must be positive"};
    }
    return {};
}

void Ssd1306DeviceConfigV2::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["i2cBusDeviceId"] = i2cBusDeviceId;
    output["i2cAddress"] = i2cAddress;
    output["width"] = width;
    output["height"] = height;
}

} // namespace ewfm
