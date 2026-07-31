#include "devices/pixel/effects/PixelEffectSolidDeviceConfig.h"

#include "devices/core/ConfigCodec.h"
#include "devices/pixel/PixelColorJson.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<PixelEffectSolidDeviceConfigV1>::value, "PixelEffectSolidDeviceConfigV1 must be POD");
static_assert(sizeof(PixelEffectSolidDeviceConfigV1) == 38U, "PixelEffectSolidDeviceConfigV1 layout changed");
static_assert(sizeof(PixelEffectSolidDeviceConfigV1::kMagic) - 1U + sizeof(PixelEffectSolidDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "PixelEffectSolidDeviceConfigV1 exceeds device config bound");

bool PixelEffectSolidDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }
    restorePreviousState = input["restorePreviousState"] | restorePreviousState;
    startupColor = parsePixelColorJson(input, "startupColor", startupColor);
    return true;
}

DeviceValidationResult PixelEffectSolidDeviceConfigV1::validate() const {
    return DeviceBaseConfigV1::validate();
}

void PixelEffectSolidDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["restorePreviousState"] = restorePreviousState;
    writePixelColorJson(output, "startupColor", startupColor);
}

bool decodePixelEffectSolidDeviceConfig(const uint8_t* blob, const size_t size, PixelEffectSolidDeviceConfigV1& config) {
    return decodeValidatedFixedConfigBlob(PixelEffectSolidDeviceConfigV1::kMagic, blob, size, config);
}

} // namespace ewfm
