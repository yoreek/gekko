#include "devices/display/oled/OledDisplayDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<OledDisplayDeviceConfigV1>::value, "OledDisplayDeviceConfigV1 must be POD");
static_assert(sizeof(OledDisplayDeviceConfigV1::kMagic) - 1U + sizeof(OledDisplayDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "OledDisplayDeviceConfigV1 exceeds device config bound");

bool encodeOledDisplayDeviceConfig(const OledDisplayDeviceConfigV1& config, uint8_t* blob, size_t capacity) {
    return encodeFixedConfigBlob(OledDisplayDeviceConfigV1::kMagic, config, blob, capacity);
}

bool decodeOledDisplayDeviceConfig(const uint8_t* blob, size_t size, OledDisplayDeviceConfigV1& config) {
    return decodeFixedConfigBlob(OledDisplayDeviceConfigV1::kMagic, blob, size, config) && config.validate().ok();
}

bool OledDisplayDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    const JsonVariantConst busDeviceId = input["i2cBusDeviceId"];
    if (!busDeviceId.isNull()) {
        if (!busDeviceId.is<unsigned long>() && !busDeviceId.is<long>() && !busDeviceId.is<int>()) {
            error = "oled display i2c bus device id must be numeric";
            return false;
        }
        const long parsed = busDeviceId.as<long>();
        if (parsed < 0 || static_cast<unsigned long>(parsed) > 0xFFFFFFFFUL) {
            error = "oled display i2c bus device id is out of bounds";
            return false;
        }
        i2cBusDeviceId = static_cast<uint32_t>(parsed);
    }

    const JsonVariantConst addressVariant = input["i2cAddress"];
    if (!addressVariant.isNull()) {
        if (!addressVariant.is<unsigned long>() && !addressVariant.is<long>() && !addressVariant.is<int>()) {
            error = "oled display i2c address must be numeric";
            return false;
        }
        const long parsed = addressVariant.as<long>();
        if (parsed < 0 || parsed > 0x7F) {
            error = "oled display i2c address is out of bounds";
            return false;
        }
        i2cAddress = static_cast<uint8_t>(parsed);
    }

    const JsonVariantConst widthVariant = input["layoutWidth"];
    if (!widthVariant.isNull()) {
        const long parsed = widthVariant.as<long>();
        if (parsed <= 0 || parsed > 255) {
            error = "oled display layout width is out of bounds";
            return false;
        }
        layoutWidth = static_cast<uint16_t>(parsed);
    }

    const JsonVariantConst heightVariant = input["layoutHeight"];
    if (!heightVariant.isNull()) {
        const long parsed = heightVariant.as<long>();
        if (parsed <= 0 || parsed > 255) {
            error = "oled display layout height is out of bounds";
            return false;
        }
        layoutHeight = static_cast<uint16_t>(parsed);
    }

    return true;
}

DeviceValidationResult OledDisplayDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (i2cBusDeviceId == 0U) {
        return {DeviceError::InvalidConfig, "oled display i2c bus device id is required"};
    }
    if (i2cAddress > 0x7FU) {
        return {DeviceError::InvalidConfig, "oled display i2c address exceeds supported range"};
    }
    if (layoutWidth == 0U || layoutHeight == 0U) {
        return {DeviceError::InvalidConfig, "oled display layout dimensions must be positive"};
    }
    return {};
}

void OledDisplayDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["i2cBusDeviceId"] = i2cBusDeviceId;
    output["i2cAddress"] = i2cAddress;
    output["layoutWidth"] = layoutWidth;
    output["layoutHeight"] = layoutHeight;
}

} // namespace ewfm
