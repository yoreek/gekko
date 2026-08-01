#include "devices/analog/input/cd74hc4067/Cd74hc4067HubDeviceConfig.h"

#include "devices/analog/adc/AdcAttenuationCodec.h"
#include "devices/analog/input/port/AnalogPortInputDeviceConfig.h"
#include "devices/core/ConfigCodec.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"

#include <type_traits>

namespace ewfm {

namespace {

bool parseUint8(const JsonVariantConst& variant, uint8_t& value) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<unsigned int>() && !variant.is<int>()) {
        return false;
    }
    const long parsed = variant.as<long>();
    if (parsed < 0 || parsed > 255L) {
        return false;
    }
    value = static_cast<uint8_t>(parsed);
    return true;
}

bool pinsOverlap(const Cd74hc4067HubDeviceConfigV1& config) {
    uint8_t pins[6]{};
    uint8_t count = 0;
    for (uint8_t select : config.selectPins) {
        pins[count++] = select;
    }
    pins[count++] = config.sigPin;
    const bool hasEnable = config.enablePin != kGpioPinUnset;
    if (hasEnable) {
        pins[count++] = config.enablePin;
    }
    for (uint8_t i = 0; i < count; ++i) {
        for (uint8_t j = i + 1; j < count; ++j) {
            if (pins[i] == pins[j]) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

static_assert(std::is_trivially_copyable<Cd74hc4067HubDeviceConfigV1>::value, "Cd74hc4067HubDeviceConfigV1 must be POD");
static_assert(sizeof(Cd74hc4067HubDeviceConfigV1::kMagic) - 1U + sizeof(Cd74hc4067HubDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Cd74hc4067HubDeviceConfigV1 exceeds device config bound");

DeviceValidationResult Cd74hc4067HubDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    for (uint8_t select : selectPins) {
        if (!gpioSwitchPinIsValid(select)) {
            return {DeviceError::InvalidConfig, "cd74hc4067 select pin is invalid"};
        }
    }
    if (enablePin != kGpioPinUnset && !gpioSwitchPinIsValid(enablePin)) {
        return {DeviceError::InvalidConfig, "cd74hc4067 enable pin is invalid"};
    }
    if (!analogPortInputGpioPinIsValid(sigPin)) {
        return {DeviceError::InvalidConfig, "cd74hc4067 sig pin is invalid"};
    }
    AdcAttenuation parsedAttenuation{};
    if (!attenuationFromByte(sigAttenuation, parsedAttenuation)) {
        return {DeviceError::InvalidConfig, "cd74hc4067 sig attenuation is invalid"};
    }
    if (pinsOverlap(*this)) {
        return {DeviceError::InvalidConfig, "cd74hc4067 pins must be distinct"};
    }
    return {};
}

bool Cd74hc4067HubDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    const JsonArrayConst selectInput = input["selectPins"].as<JsonArrayConst>();
    if (!selectInput.isNull()) {
        if (selectInput.size() != 4U) {
            error = "cd74hc4067 select pins must have exactly 4 entries";
            return false;
        }
        uint8_t index = 0;
        for (JsonVariantConst entry : selectInput) {
            uint8_t pin = selectPins[index];
            if (!parseUint8(entry, pin)) {
                error = "cd74hc4067 select pin must be numeric";
                return false;
            }
            selectPins[index] = pin;
            ++index;
        }
    }

    uint8_t enable = enablePin;
    if (!parseUint8(input["enablePin"], enable)) {
        error = "cd74hc4067 enable pin must be numeric";
        return false;
    }
    enablePin = enable;

    uint8_t sig = sigPin;
    if (!parseUint8(input["sigPin"], sig)) {
        error = "cd74hc4067 sig pin must be numeric";
        return false;
    }
    sigPin = sig;

    AdcAttenuation parsedAttenuation{};
    if (!attenuationFromByte(sigAttenuation, parsedAttenuation)) {
        parsedAttenuation = AdcAttenuation::Db11;
    }
    if (!attenuationFromString(input["sigAttenuation"] | attenuationName(parsedAttenuation), parsedAttenuation)) {
        error = "cd74hc4067 sig attenuation is invalid";
        return false;
    }
    sigAttenuation = static_cast<uint8_t>(parsedAttenuation);

    const DeviceValidationResult result = validate();
    if (!result.ok()) {
        error = result.message;
        return false;
    }
    return true;
}

void Cd74hc4067HubDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    JsonArray selectOutput = output.createNestedArray("selectPins");
    for (uint8_t select : selectPins) {
        selectOutput.add(select);
    }
    output["enablePin"] = enablePin;
    output["sigPin"] = sigPin;
    AdcAttenuation parsedAttenuation{};
    (void)attenuationFromByte(sigAttenuation, parsedAttenuation);
    output["sigAttenuation"] = attenuationName(parsedAttenuation);
}

} // namespace ewfm
