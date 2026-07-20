#include "devices/analog/input/AnalogInputPollConfig.h"

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

bool parseUint16(const JsonVariantConst& variant, uint16_t& value) {
    uint32_t parsed = value;
    if (!parseUint32(variant, parsed) || parsed > 65535UL) {
        return false;
    }
    value = static_cast<uint16_t>(parsed);
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

bool parseBoolField(const JsonObjectConst& input, const char* key, bool defaultValue) {
    return (input[key] | defaultValue) ? true : false;
}

} // namespace

DeviceValidationResult AnalogInputPollConfigV1::validate(uint8_t minAdcSamples, uint8_t maxAdcSamples) const {
    if (adcSamples < minAdcSamples || adcSamples > maxAdcSamples) {
        return {DeviceError::InvalidConfig, "analog input adc sample count is invalid"};
    }
    if (reportAlways > 1U) {
        return {DeviceError::InvalidConfig, "analog input report policy is invalid"};
    }
    if (reportDeltaMilliVolts == 0U) {
        return {DeviceError::InvalidConfig, "analog input report delta is invalid"};
    }
    if (pollMs < kAnalogInputPollConfigMinPollMs || pollMs > kAnalogInputPollConfigMaxPollMs) {
        return {DeviceError::InvalidConfig, "analog input poll period is invalid"};
    }
    return {};
}

bool AnalogInputPollConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    reportAlways = parseBoolField(input, "reportAlways", reportAlways != 0U) ? 1U : 0U;

    uint8_t samples = adcSamples;
    if (!parseUint8(input["adcSamples"], samples)) {
        error = "analog input adc sample count must be numeric";
        return false;
    }
    adcSamples = samples;

    uint32_t poll = pollMs;
    if (!parseUint32(input["pollMs"], poll)) {
        error = "analog input poll period must be numeric";
        return false;
    }
    pollMs = poll;

    uint16_t reportDelta = reportDeltaMilliVolts;
    if (!parseUint16(input["reportDeltaMilliVolts"], reportDelta)) {
        error = "analog input report delta must be numeric";
        return false;
    }
    reportDeltaMilliVolts = reportDelta;
    return true;
}

void AnalogInputPollConfigV1::writeJson(JsonObject output) const {
    output["adcSamples"] = adcSamples;
    output["pollMs"] = pollMs;
    output["reportDeltaMilliVolts"] = reportDeltaMilliVolts;
    output["reportAlways"] = reportAlways != 0U;
}

} // namespace ewfm
