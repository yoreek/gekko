#include "devices/switch/SwitchDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <type_traits>

namespace ewfm {

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
    const DeviceValidationResult baseValidation = validateDeviceBaseConfig(config.base);
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    OutputState state{};
    if (!outputStateFromByte(config.startupState, state)) {
        return {DeviceError::InvalidConfig, "switch startup state is invalid"};
    }
    if (!outputStateFromByte(config.safeState, state)) {
        return {DeviceError::InvalidConfig, "switch safe state is invalid"};
    }
    return {};
}

bool switchConfigStartupState(const SwitchDeviceConfigV1& config, OutputState& state) {
    return outputStateFromByte(config.startupState, state);
}

bool switchConfigSafeState(const SwitchDeviceConfigV1& config, OutputState& state) {
    return outputStateFromByte(config.safeState, state);
}

} // namespace ewfm
