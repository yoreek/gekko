#include "devices/pixel/PixelStripDeviceConfig.h"

#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceTypes.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<PixelStripDeviceConfigV1>::value, "PixelStripDeviceConfigV1 must be POD");
static_assert(sizeof(PixelStripDeviceConfigV1) == 39U, "PixelStripDeviceConfigV1 layout changed");
static_assert(sizeof(PixelStripDeviceConfigV1::kMagic) - 1U + sizeof(PixelStripDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "PixelStripDeviceConfigV1 exceeds device config bound");

namespace {
// Same output-capable-pin rule LedcAnalogOutputDeviceConfig uses: excludes input-only ADC2 pins
// (34-39) and the flash-strapping pins (6-11) on the classic ESP32.
bool isPixelStripPinValid(const uint8_t pin) {
    if (pin == 0xFFU) {
        return false;
    }
    if (pin >= 34U && pin <= 39U) {
        return false;
    }
    if (pin >= 6U && pin <= 11U) {
        return false;
    }
    return pin <= 48U;
}

bool parseUnsignedJson(const JsonVariantConst& input, const char*& error, const char* typeError, const char* rangeError,
                       const uint32_t minValue, const uint32_t maxValue, uint32_t& outValue) {
    if (input.isNull()) {
        return true;
    }
    if (!input.is<unsigned long>() && !input.is<long>() && !input.is<int>()) {
        error = typeError;
        return false;
    }
    const uint32_t parsed = static_cast<uint32_t>(input.as<unsigned long>());
    if (parsed < minValue || parsed > maxValue) {
        error = rangeError;
        return false;
    }
    outValue = parsed;
    return true;
}
} // namespace

bool PixelStripDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    uint32_t value = pin;
    if (!parseUnsignedJson(input["pin"], error, "pixel strip pin must be numeric", "pixel strip pin is out of bounds", 0U, 255U, value)) {
        return false;
    }
    pin = static_cast<uint8_t>(value);

    value = pixelCount;
    if (!parseUnsignedJson(input["pixelCount"], error, "pixel strip pixel count must be numeric",
                           "pixel strip pixel count is out of bounds", 1U, static_cast<uint32_t>(kMaxPixelStripLength), value)) {
        return false;
    }
    pixelCount = static_cast<uint16_t>(value);

    restorePreviousState = input["restorePreviousState"] | restorePreviousState;

    value = pixelBrightnessToPercent(startupBrightness);
    if (!parseUnsignedJson(input["startupBrightness"], error, "pixel strip startup brightness must be numeric",
                           "pixel strip startup brightness is out of bounds", 0U, 100U, value)) {
        return false;
    }
    startupBrightness = percentToPixelBrightness(static_cast<uint8_t>(value));
    return true;
}

DeviceValidationResult PixelStripDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (!isPixelStripPinValid(pin)) {
        return {DeviceError::InvalidConfig, "pixel strip pin is invalid"};
    }
    if (pixelCount == 0U || pixelCount > kMaxPixelStripLength) {
        return {DeviceError::InvalidConfig, "pixel strip pixel count is out of bounds"};
    }
    return {};
}

void PixelStripDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["pin"] = pin;
    output["pixelCount"] = pixelCount;
    output["restorePreviousState"] = restorePreviousState;
    output["startupBrightness"] = pixelBrightnessToPercent(startupBrightness);
}

bool decodePixelStripDeviceConfig(const uint8_t* blob, const size_t size, PixelStripDeviceConfigV1& config) {
    return decodeValidatedFixedConfigBlob(PixelStripDeviceConfigV1::kMagic, blob, size, config);
}

} // namespace ewfm
