#include "devices/analog/input/channel/AnalogInputChannelDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

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

static_assert(std::is_trivially_copyable<AnalogInputChannelDeviceConfigV1>::value, "AnalogInputChannelDeviceConfigV1 must be POD");
static_assert(sizeof(AnalogInputChannelDeviceConfigV1::kMagic) - 1U + sizeof(AnalogInputChannelDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "AnalogInputChannelDeviceConfigV1 exceeds device config bound");

DeviceValidationResult AnalogInputChannelDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (channel > kAnalogInputChannelMaxChannel) {
        return {DeviceError::InvalidConfig, "analog input channel is invalid"};
    }
    return poll.validate(kAnalogInputChannelMinAdcSamples, kAnalogInputChannelMaxAdcSamples);
}

bool AnalogInputChannelDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    uint8_t ch = channel;
    if (!parseUint8(input["channel"], ch)) {
        error = "analog input channel must be numeric";
        return false;
    }
    channel = ch;

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

void AnalogInputChannelDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["channel"] = channel;
    poll.writeJson(output);
}

} // namespace ewfm
