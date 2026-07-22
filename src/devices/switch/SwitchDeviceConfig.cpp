#include "devices/switch/SwitchDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <type_traits>

namespace ewfm {

EWFM_LEGACY_CONFIG_USE_BEGIN
static_assert(std::is_trivially_copyable<SwitchDeviceConfigV1>::value, "SwitchDeviceConfigV1 must be POD");
static_assert(sizeof(SwitchDeviceConfigV1) == 38, "SwitchDeviceConfigV1 layout changed");
static_assert(sizeof(SwitchDeviceConfigV1::kMagic) - 1U + sizeof(SwitchDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "SwitchDeviceConfigV1 exceeds device config bound");
EWFM_LEGACY_CONFIG_USE_END
static_assert(std::is_trivially_copyable<SwitchDeviceConfigV2>::value, "SwitchDeviceConfigV2 must be POD");
static_assert(sizeof(SwitchDeviceConfigV2) == 38, "SwitchDeviceConfigV2 layout changed");

bool OutputDeviceValueCodec<bool>::parseJson(const JsonVariantConst& input, bool& state, const char*& error) {
    if (!input.is<bool>()) {
        error = "output state value must be boolean";
        return false;
    }
    state = input.as<bool>();
    return true;
}

bool OutputDeviceValueCodec<bool>::valid(const bool state) {
    (void)state;
    return true;
}

void OutputDeviceValueCodec<bool>::writeJson(JsonObject output, const char* key, const bool state) {
    output[key] = state;
}

EWFM_LEGACY_CONFIG_USE_BEGIN
DeviceValidationResult SwitchDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (startupState > 2U) {
        return {DeviceError::InvalidConfig, "output startup state is invalid"};
    }
    if (safeState > 2U) {
        return {DeviceError::InvalidConfig, "output safe state is invalid"};
    }
    return {};
}

void SwitchDeviceConfigV2::migrateFrom(const SwitchDeviceConfigV1& legacy) {
    static_cast<DeviceBaseConfigV1&>(*this) = legacy;
    restorePreviousState = legacy.restorePreviousState;
    startupState = legacy.startupState == 1U;
    safeState = legacy.safeState == 1U;
    inverted = legacy.inverted;
}
EWFM_LEGACY_CONFIG_USE_END

} // namespace ewfm
