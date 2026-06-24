#include "devices/switch/SwitchDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
bool parseOutputState(const char* value, OutputState& state, const char*& error) {
    if (value == nullptr || std::strcmp(value, "off") == 0) {
        state = OutputState::Off;
        return true;
    }
    if (std::strcmp(value, "on") == 0) {
        state = OutputState::On;
        return true;
    }
    if (std::strcmp(value, "disabled") == 0) {
        state = OutputState::Disabled;
        return true;
    }
    error = "switch output state is invalid";
    return false;
}
} // namespace

static_assert(std::is_trivially_copyable<SwitchDeviceConfigV1>::value, "SwitchDeviceConfigV1 must be POD");
static_assert(sizeof(SwitchDeviceConfigV1) == 38, "SwitchDeviceConfigV1 layout changed");
static_assert(sizeof(SwitchDeviceConfigV1::kMagic) - 1U + sizeof(SwitchDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "SwitchDeviceConfigV1 exceeds device config bound");

bool encodeSwitchDeviceConfig(const SwitchDeviceConfigV1& config, uint8_t* blob, size_t capacity) {
    return encodeFixedConfigBlob(SwitchDeviceConfigV1::kMagic, config, blob, capacity);
}

bool decodeSwitchDeviceConfig(const uint8_t* blob, size_t size, SwitchDeviceConfigV1& config) {
    return decodeFixedConfigBlob(SwitchDeviceConfigV1::kMagic, blob, size, config) && validateSwitchDeviceConfig(config).ok();
}

DeviceValidationResult validateSwitchDeviceConfig(const SwitchDeviceConfigV1& config) {
    const DeviceValidationResult baseValidation = config.validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (!outputStateIsValid(config.startupState)) {
        return {DeviceError::InvalidConfig, "switch startup state is invalid"};
    }
    if (!outputStateIsValid(config.safeState)) {
        return {DeviceError::InvalidConfig, "switch safe state is invalid"};
    }
    return {};
}

bool SwitchDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }
    restorePreviousState = (input["restorePreviousState"] | false) ? true : false;
    inverted = (input["inverted"] | false) ? true : false;

    if (!parseOutputState(input["startupState"] | "off", startupState, error)) {
        return false;
    }
    if (!parseOutputState(input["safeState"] | "off", safeState, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult SwitchDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (!outputStateIsValid(startupState)) {
        return {DeviceError::InvalidConfig, "switch startup state is invalid"};
    }
    if (!outputStateIsValid(safeState)) {
        return {DeviceError::InvalidConfig, "switch safe state is invalid"};
    }
    return {};
}

void SwitchDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["restorePreviousState"] = restorePreviousState;
    output["startupState"] = outputStateName(startupState);
    output["safeState"] = outputStateName(safeState);
    output["inverted"] = inverted;
}

bool switchConfigStartupState(const SwitchDeviceConfigV1& config, OutputState& state) {
    state = config.startupState;
    return true;
}

bool switchConfigSafeState(const SwitchDeviceConfigV1& config, OutputState& state) {
    state = config.safeState;
    return true;
}

} // namespace ewfm
