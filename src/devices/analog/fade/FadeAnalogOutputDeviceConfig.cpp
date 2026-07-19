#include "devices/analog/fade/FadeAnalogOutputDeviceConfig.h"

#include "devices/analog/AnalogOutputDeviceConfig.h"
#include "devices/core/ConfigCodec.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<FadeAnalogOutputDeviceConfigV1>::value, "FadeAnalogOutputDeviceConfigV1 must be POD");
static_assert(sizeof(FadeAnalogOutputDeviceConfigV1) == 40U, "FadeAnalogOutputDeviceConfigV1 layout changed");
static_assert(sizeof(FadeAnalogOutputDeviceConfigV1::kMagic) - 1U + sizeof(FadeAnalogOutputDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "FadeAnalogOutputDeviceConfigV1 exceeds device config bound");

bool FadeAnalogOutputDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }
    if (!input["maxStep"].isNull() && !OutputDeviceValueCodec<uint16_t>::parseJson(input["maxStep"], maxStep, error)) {
        return false;
    }
    const JsonVariantConst interval = input["stepIntervalMs"];
    if (!interval.isNull()) {
        if (!interval.is<unsigned long>() && !interval.is<long>() && !interval.is<int>()) {
            error = "fade step interval must be numeric";
            return false;
        }
        const long parsed = interval.as<long>();
        if (parsed < 1L || parsed > 60000L) {
            error = "fade step interval is out of bounds";
            return false;
        }
        stepIntervalMs = static_cast<uint32_t>(parsed);
    }
    return true;
}

DeviceValidationResult FadeAnalogOutputDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (maxStep == 0U || maxStep > kAnalogOutputLevelMax) {
        return {DeviceError::InvalidConfig, "fade max step is out of bounds"};
    }
    if (stepIntervalMs == 0U || stepIntervalMs > 60000U) {
        return {DeviceError::InvalidConfig, "fade step interval is out of bounds"};
    }
    return {};
}

void FadeAnalogOutputDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    OutputDeviceValueCodec<uint16_t>::writeJson(output, "maxStep", maxStep);
    output["stepIntervalMs"] = stepIntervalMs;
}

bool decodeFadeAnalogOutputDeviceConfig(const uint8_t* blob, const size_t size, FadeAnalogOutputDeviceConfigV1& config) {
    return decodeValidatedFixedConfigBlob(FadeAnalogOutputDeviceConfigV1::kMagic, blob, size, config);
}

} // namespace ewfm
