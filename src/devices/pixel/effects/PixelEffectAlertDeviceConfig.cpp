#include "devices/pixel/effects/PixelEffectAlertDeviceConfig.h"

#include "devices/core/ConfigCodec.h"
#include "devices/pixel/PixelColorJson.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<PixelEffectAlertDeviceConfigV1>::value, "PixelEffectAlertDeviceConfigV1 must be POD");
static_assert(sizeof(PixelEffectAlertDeviceConfigV1) == 41U, "PixelEffectAlertDeviceConfigV1 layout changed");
static_assert(sizeof(PixelEffectAlertDeviceConfigV1::kMagic) - 1U + sizeof(PixelEffectAlertDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "PixelEffectAlertDeviceConfigV1 exceeds device config bound");

namespace {
bool parseBlinkIntervalMs(const JsonVariantConst& input, const char*& error, uint32_t& outValue) {
    if (input.isNull()) {
        return true;
    }
    if (!input.is<unsigned long>() && !input.is<long>() && !input.is<int>()) {
        error = "pixel effect alert blink interval must be numeric";
        return false;
    }
    const uint32_t parsed = static_cast<uint32_t>(input.as<unsigned long>());
    if (parsed < kPixelEffectAlertMinBlinkIntervalMs || parsed > kPixelEffectAlertMaxBlinkIntervalMs) {
        error = "pixel effect alert blink interval is out of bounds";
        return false;
    }
    outValue = parsed;
    return true;
}
} // namespace

bool PixelEffectAlertDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }
    color = parsePixelColorJson(input, "color", color);
    return parseBlinkIntervalMs(input["blinkIntervalMs"], error, blinkIntervalMs);
}

DeviceValidationResult PixelEffectAlertDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (blinkIntervalMs < kPixelEffectAlertMinBlinkIntervalMs || blinkIntervalMs > kPixelEffectAlertMaxBlinkIntervalMs) {
        return {DeviceError::InvalidConfig, "pixel effect alert blink interval is out of bounds"};
    }
    return {};
}

void PixelEffectAlertDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    writePixelColorJson(output, "color", color);
    output["blinkIntervalMs"] = blinkIntervalMs;
}

bool decodePixelEffectAlertDeviceConfig(const uint8_t* blob, const size_t size, PixelEffectAlertDeviceConfigV1& config) {
    return decodeValidatedFixedConfigBlob(PixelEffectAlertDeviceConfigV1::kMagic, blob, size, config);
}

} // namespace ewfm
