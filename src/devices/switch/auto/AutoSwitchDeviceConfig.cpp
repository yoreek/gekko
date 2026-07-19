#include "devices/switch/auto/AutoSwitchDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
bool parsePauseDurationSeconds(const JsonVariantConst& variant, uint32_t& value, const char*& error) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<unsigned long>() && !variant.is<unsigned int>() && !variant.is<int>() && !variant.is<long>()) {
        error = "auto switch pauseDurationSeconds must be numeric";
        return false;
    }
    const long parsed = variant.as<long>();
    if (parsed < 0) {
        error = "auto switch pauseDurationSeconds must be non-negative";
        return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}
} // namespace

static_assert(std::is_trivially_copyable<AutoSwitchDeviceConfigV1>::value, "AutoSwitchDeviceConfigV1 must be POD");
static_assert(sizeof(AutoSwitchDeviceConfigV1::kMagic) - 1U + sizeof(AutoSwitchDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "AutoSwitchDeviceConfigV1 exceeds device config bound");

bool autoSwitchModeFromString(const char* value, AutoSwitchMode& mode) {
    if (value == nullptr || std::strcmp(value, "auto") == 0) {
        mode = AutoSwitchMode::Auto;
        return true;
    }
    if (std::strcmp(value, "on") == 0) {
        mode = AutoSwitchMode::On;
        return true;
    }
    if (std::strcmp(value, "off") == 0) {
        mode = AutoSwitchMode::Off;
        return true;
    }
    if (std::strcmp(value, "paused") == 0) {
        mode = AutoSwitchMode::Paused;
        return true;
    }
    return false;
}

const char* autoSwitchModeName(AutoSwitchMode mode) {
    switch (mode) {
    case AutoSwitchMode::Off:
        return "off";
    case AutoSwitchMode::On:
        return "on";
    case AutoSwitchMode::Auto:
        return "auto";
    case AutoSwitchMode::Paused:
        return "paused";
    }
    return "auto";
}

bool parseAutoSwitchDeviceConfigJson(const JsonObjectConst& input, AutoSwitchDeviceConfigV1& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeAutoSwitchDeviceConfigJson(const AutoSwitchDeviceConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

bool AutoSwitchDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }
    return parsePauseDurationSeconds(input["pauseDurationSeconds"], pauseDurationSeconds, error);
}

DeviceValidationResult AutoSwitchDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (pauseDurationSeconds < kAutoSwitchMinPauseDurationSeconds || pauseDurationSeconds > kAutoSwitchMaxPauseDurationSeconds) {
        return {DeviceError::InvalidConfig, "auto switch pauseDurationSeconds is invalid"};
    }
    return {};
}

void AutoSwitchDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["pauseDurationSeconds"] = pauseDurationSeconds;
}

} // namespace ewfm
