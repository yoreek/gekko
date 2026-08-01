#include "devices/analog/input/port/AnalogPortInputDeviceConfig.h"

#include "devices/analog/adc/AdcAttenuationCodec.h"
#include "devices/core/ConfigCodec.h"
#include "platform/BoardPinCapabilities.h"

#include <type_traits>

namespace ewfm {

namespace {

bool parseUint32(const JsonVariantConst& variant, uint32_t& value) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<unsigned long>() && !variant.is<unsigned int>() && !variant.is<int>()) {
        return false;
    }
    const long parsed = variant.as<long>();
    if (parsed < 0) {
        return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool parseUint8(const JsonVariantConst& variant, uint8_t& value) {
    uint32_t parsed = value;
    if (!parseUint32(variant, parsed) || parsed > 255UL) {
        return false;
    }
    value = static_cast<uint8_t>(parsed);
    return true;
}

} // namespace

static_assert(std::is_trivially_copyable<AnalogPortInputDeviceConfigV1>::value, "AnalogPortInputDeviceConfigV1 must be POD");
static_assert(sizeof(AnalogPortInputDeviceConfigV1::kMagic) - 1U + sizeof(AnalogPortInputDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "AnalogPortInputDeviceConfigV1 exceeds device config bound");

bool analogPortInputGpioPinIsValid(uint8_t pin) {
    // ADC1-capable pins on the board. ADC2 pins are excluded because the ADC2 peripheral is
    // unusable while WiFi is active, and WiFi is always active on this project.
    return boardPinHasRole(pin, kPinRoleAdc1);
}

DeviceValidationResult AnalogPortInputDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (!analogPortInputGpioPinIsValid(gpioPin)) {
        return {DeviceError::InvalidConfig, "analog port input gpio pin is invalid"};
    }
    AdcAttenuation parsedAttenuation{};
    if (!attenuationFromByte(attenuation, parsedAttenuation)) {
        return {DeviceError::InvalidConfig, "analog port input attenuation is invalid"};
    }
    return poll.validate(kAnalogPortInputMinAdcSamples, kAnalogPortInputMaxAdcSamples);
}

bool AnalogPortInputDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    uint8_t pin = gpioPin;
    if (!parseUint8(input["gpioPin"], pin)) {
        error = "analog port input gpio pin must be numeric";
        return false;
    }
    gpioPin = pin;

    AdcAttenuation parsedAttenuation{};
    if (!attenuationFromByte(attenuation, parsedAttenuation)) {
        parsedAttenuation = AdcAttenuation::Db11;
    }
    if (!attenuationFromString(input["attenuation"] | attenuationName(parsedAttenuation), parsedAttenuation)) {
        error = "analog port input attenuation is invalid";
        return false;
    }
    attenuation = static_cast<uint8_t>(parsedAttenuation);

    if (!poll.parseJson(input, error)) {
        return false;
    }

    const DeviceValidationResult result = validate();
    if (!result.ok()) {
        error = result.message;
        return false;
    }
    return true;
}

void AnalogPortInputDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["gpioPin"] = gpioPin;
    AdcAttenuation parsedAttenuation{};
    (void)attenuationFromByte(attenuation, parsedAttenuation);
    output["attenuation"] = attenuationName(parsedAttenuation);
    poll.writeJson(output);
}

} // namespace ewfm
