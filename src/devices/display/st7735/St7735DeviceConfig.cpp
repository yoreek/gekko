#include "devices/display/st7735/St7735DeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<St7735DeviceConfigV1>::value, "St7735DeviceConfigV1 must be POD");
static_assert(sizeof(St7735DeviceConfigV1::kMagic) - 1U + sizeof(St7735DeviceConfigV1) <= kMaxDeviceConfigBytes,
              "St7735DeviceConfigV1 exceeds device config bound");

bool encodeSt7735DeviceConfig(const St7735DeviceConfigV1& config, uint8_t* blob, size_t capacity) {
    return encodeFixedConfigBlob(St7735DeviceConfigV1::kMagic, config, blob, capacity);
}

bool decodeSt7735DeviceConfig(const uint8_t* blob, size_t size, St7735DeviceConfigV1& config) {
    return decodeFixedConfigBlob(St7735DeviceConfigV1::kMagic, blob, size, config) && config.validate().ok();
}

bool St7735DeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
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

    const JsonVariantConst widthVariant = input["layoutWidth"];
    if (!widthVariant.isNull()) {
        const long parsed = widthVariant.as<long>();
        if (parsed <= 0 || parsed > 255) {
            error = "st7735 layout width is out of bounds";
            return false;
        }
        layoutWidth = static_cast<uint16_t>(parsed);
    }

    const JsonVariantConst heightVariant = input["layoutHeight"];
    if (!heightVariant.isNull()) {
        const long parsed = heightVariant.as<long>();
        if (parsed <= 0 || parsed > 255) {
            error = "st7735 layout height is out of bounds";
            return false;
        }
        layoutHeight = static_cast<uint16_t>(parsed);
    }

    return true;
}

DeviceValidationResult St7735DeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (spiBusDeviceId == 0U) {
        return {DeviceError::InvalidConfig, "st7735 spi bus device id is required"};
    }
    if (layoutWidth == 0U || layoutHeight == 0U) {
        return {DeviceError::InvalidConfig, "st7735 layout dimensions must be positive"};
    }
    return {};
}

void St7735DeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["spiBusDeviceId"] = spiBusDeviceId;
    output["chipSelectPin"] = chipSelectPin;
    output["layoutWidth"] = layoutWidth;
    output["layoutHeight"] = layoutHeight;
}

} // namespace ewfm
