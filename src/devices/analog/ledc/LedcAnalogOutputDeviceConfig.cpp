#include "devices/analog/ledc/LedcAnalogOutputDeviceConfig.h"

#include "devices/core/ConfigCodec.h"
#include "platform/BoardPinCapabilities.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<LedcAnalogOutputDeviceConfigV1>::value, "LedcAnalogOutputDeviceConfigV1 must be POD");
static_assert(sizeof(LedcAnalogOutputDeviceConfigV1) == 47U, "LedcAnalogOutputDeviceConfigV1 layout changed");
static_assert(sizeof(LedcAnalogOutputDeviceConfigV1::kMagic) - 1U + sizeof(LedcAnalogOutputDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "LedcAnalogOutputDeviceConfigV1 exceeds device config bound");

namespace {
constexpr uint32_t kMaxLedcFrequencyHz = 1000000U;
constexpr uint8_t kMinLedcDutyBits = 1U;
constexpr uint8_t kMaxLedcDutyBits = 20U;

bool parseUnsignedJson(const JsonVariantConst& input, const char*& error, const char* typeError, const char* rangeError,
                       const uint32_t maxValue, uint32_t& outValue) {
    if (input.isNull()) {
        return false;
    }
    if (!input.is<unsigned long>() && !input.is<long>() && !input.is<int>()) {
        error = typeError;
        return false;
    }
    const uint32_t parsed = static_cast<uint32_t>(input.as<unsigned long>());
    if (parsed > maxValue) {
        error = rangeError;
        return false;
    }
    outValue = parsed;
    return true;
}

bool isOutputCapablePin(const uint8_t pin) {
    return boardPinHasRole(pin, kPinRoleOutput);
}

bool parseOptionalUnsignedJson(const JsonVariantConst& input, const char*& error, const char* typeError, const char* rangeError,
                               const uint32_t maxValue, uint32_t& outValue) {
    if (input.isNull()) {
        return true;
    }
    return parseUnsignedJson(input, error, typeError, rangeError, maxValue, outValue);
}

bool parseLedcHardwareConfig(const JsonObjectConst& input, uint8_t& pin, uint8_t& ledcChannel, uint32_t& frequencyHz, uint8_t& dutyBits,
                             const char*& error) {
    const auto parseField = [&error](const JsonVariantConst& field, const char* typeError, const char* rangeError, const uint32_t maxValue,
                                     uint32_t& value) {
        return parseOptionalUnsignedJson(field, error, typeError, rangeError, maxValue, value);
    };

    uint32_t value = pin;
    if (!parseField(input["pin"], "ledc output pin must be numeric", "ledc output pin is out of bounds", 255U, value)) {
        return false;
    }
    pin = static_cast<uint8_t>(value);

    value = ledcChannel;
    if (!parseField(input["ledcChannel"], "ledc channel must be numeric", "ledc channel is out of bounds", 255U, value)) {
        return false;
    }
    ledcChannel = static_cast<uint8_t>(value);

    if (!parseField(input["frequencyHz"], "ledc frequency must be numeric", "ledc frequency is out of bounds", kMaxLedcFrequencyHz,
                    frequencyHz)) {
        return false;
    }

    value = dutyBits;
    if (!parseField(input["dutyBits"], "ledc duty bits must be numeric", "ledc duty bits is out of bounds", kMaxLedcDutyBits, value)) {
        return false;
    }
    dutyBits = static_cast<uint8_t>(value);
    return true;
}

DeviceValidationResult validateLedcHardwareConfig(const uint8_t pin, const uint8_t ledcChannel, const uint32_t frequencyHz,
                                                  const uint8_t dutyBits) {
    if (!isOutputCapablePin(pin)) {
        return {DeviceError::InvalidConfig, "ledc output pin is invalid"};
    }
    if (ledcChannel > 15U) {
        return {DeviceError::InvalidConfig, "ledc channel is out of bounds"};
    }
    if (frequencyHz == 0U || frequencyHz > kMaxLedcFrequencyHz) {
        return {DeviceError::InvalidConfig, "ledc frequency is out of bounds"};
    }
    if (dutyBits < kMinLedcDutyBits || dutyBits > kMaxLedcDutyBits) {
        return {DeviceError::InvalidConfig, "ledc duty bits is out of bounds"};
    }
    return {};
}

void writeLedcHardwareConfig(JsonObject output, const uint8_t pin, const uint8_t ledcChannel, const uint32_t frequencyHz,
                             const uint8_t dutyBits) {
    output["pin"] = pin;
    output["ledcChannel"] = ledcChannel;
    output["frequencyHz"] = frequencyHz;
    output["dutyBits"] = dutyBits;
}
} // namespace

bool LedcAnalogOutputDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    return AnalogOutputDeviceConfigV1::parseJson(input, error) &&
           parseLedcHardwareConfig(input, pin, ledcChannel, frequencyHz, dutyBits, error);
}

DeviceValidationResult LedcAnalogOutputDeviceConfigV1::validate() const {
    const DeviceValidationResult analogValidation = AnalogOutputDeviceConfigV1::validate();
    return analogValidation.ok() ? validateLedcHardwareConfig(pin, ledcChannel, frequencyHz, dutyBits) : analogValidation;
}

void LedcAnalogOutputDeviceConfigV1::writeJson(JsonObject output) const {
    AnalogOutputDeviceConfigV1::writeJson(output);
    writeLedcHardwareConfig(output, pin, ledcChannel, frequencyHz, dutyBits);
}

bool decodeLedcAnalogOutputDeviceConfig(const uint8_t* blob, const size_t size, LedcAnalogOutputDeviceConfigV1& config) {
    return decodeValidatedFixedConfigBlob(LedcAnalogOutputDeviceConfigV1::kMagic, blob, size, config);
}

} // namespace ewfm
