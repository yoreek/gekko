#include "devices/sensors/binary/BinarySensorDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
// Same flash-pin exclusion as gpioSwitchPinIsValid, but inputs may also use the input-only pins
// 34-39 (which the switch validator caps out at 33).
constexpr uint8_t kMaxEsp32InputPin = 39;
constexpr uint8_t kFirstInputOnlyPin = 34;
constexpr uint8_t kFlashPinStart = 6;
constexpr uint8_t kFlashPinEnd = 11;

bool gpioInputPullModeFromByte(uint8_t value, GpioInputPullMode& pullMode) {
    switch (value) {
    case static_cast<uint8_t>(GpioInputPullMode::None):
        pullMode = GpioInputPullMode::None;
        return true;
    case static_cast<uint8_t>(GpioInputPullMode::PullUp):
        pullMode = GpioInputPullMode::PullUp;
        return true;
    case static_cast<uint8_t>(GpioInputPullMode::PullDown):
        pullMode = GpioInputPullMode::PullDown;
        return true;
    default:
        return false;
    }
}
} // namespace

static_assert(std::is_trivially_copyable<BinarySensorDeviceConfigV1>::value, "BinarySensorDeviceConfigV1 must be POD");
static_assert(sizeof(BinarySensorDeviceConfigV1::kMagic) - 1U + sizeof(BinarySensorDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "BinarySensorDeviceConfigV1 exceeds device config bound");

bool gpioInputPullModeFromString(const char* value, GpioInputPullMode& pullMode) {
    if (value == nullptr || std::strcmp(value, "none") == 0) {
        pullMode = GpioInputPullMode::None;
        return true;
    }
    if (std::strcmp(value, "pullup") == 0) {
        pullMode = GpioInputPullMode::PullUp;
        return true;
    }
    if (std::strcmp(value, "pulldown") == 0) {
        pullMode = GpioInputPullMode::PullDown;
        return true;
    }
    return false;
}

const char* gpioInputPullModeName(GpioInputPullMode pullMode) {
    switch (pullMode) {
    case GpioInputPullMode::None:
        return "none";
    case GpioInputPullMode::PullUp:
        return "pullup";
    case GpioInputPullMode::PullDown:
        return "pulldown";
    }
    return "none";
}

bool binarySensorPinIsValid(uint8_t pin) {
    if (pin > kMaxEsp32InputPin) {
        return false;
    }
    return pin < kFlashPinStart || pin > kFlashPinEnd;
}

// GPIO 34-39 are input-only pins without internal pull resistors on the ESP32.
bool binarySensorPinSupportsPull(uint8_t pin) {
    return pin < kFirstInputOnlyPin;
}

bool BinarySensorDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    gpioPin = static_cast<uint8_t>(input["gpioPin"] | static_cast<int>(gpioPin));
    if (!binarySensorPinIsValid(gpioPin)) {
        error = "binary sensor pin is invalid";
        return false;
    }

    GpioInputPullMode parsedPullMode{};
    (void)gpioInputPullModeFromByte(pullMode, parsedPullMode);
    const JsonVariantConst pullModeVariant = input["pullMode"];
    if (!pullModeVariant.isNull()) {
        if (!pullModeVariant.is<const char*>()) {
            error = "binary sensor pullMode must be a string";
            return false;
        }
        if (!gpioInputPullModeFromString(pullModeVariant.as<const char*>(), parsedPullMode)) {
            error = "binary sensor pullMode is invalid";
            return false;
        }
    }
    pullMode = static_cast<uint8_t>(parsedPullMode);

    inverted = (input["inverted"] | (inverted != 0U)) ? 1U : 0U;

    const JsonVariantConst debounceVariant = input["debounceMs"];
    if (!debounceVariant.isNull()) {
        if (!debounceVariant.is<unsigned int>() && !debounceVariant.is<int>() && !debounceVariant.is<long>()) {
            error = "binary sensor debounceMs must be numeric";
            return false;
        }
        const long parsedDebounce = debounceVariant.as<long>();
        if (parsedDebounce < 0 || parsedDebounce > static_cast<long>(kBinarySensorMaxDebounceMs)) {
            error = "binary sensor debounceMs is out of range";
            return false;
        }
        debounceMs = static_cast<uint16_t>(parsedDebounce);
    }
    return true;
}

DeviceValidationResult BinarySensorDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (!binarySensorPinIsValid(gpioPin)) {
        return {DeviceError::InvalidConfig, "binary sensor pin is invalid"};
    }
    GpioInputPullMode parsedPullMode{};
    if (!gpioInputPullModeFromByte(pullMode, parsedPullMode)) {
        return {DeviceError::InvalidConfig, "binary sensor pullMode is invalid"};
    }
    if (parsedPullMode != GpioInputPullMode::None && !binarySensorPinSupportsPull(gpioPin)) {
        return {DeviceError::InvalidConfig, "binary sensor pin has no internal pull resistors"};
    }
    if (debounceMs > kBinarySensorMaxDebounceMs) {
        return {DeviceError::InvalidConfig, "binary sensor debounceMs is out of range"};
    }
    return {};
}

void BinarySensorDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    GpioInputPullMode parsedPullMode{};
    (void)gpioInputPullModeFromByte(pullMode, parsedPullMode);
    output["gpioPin"] = gpioPin;
    output["pullMode"] = gpioInputPullModeName(parsedPullMode);
    output["inverted"] = inverted != 0U;
    output["debounceMs"] = debounceMs;
}

} // namespace ewfm
