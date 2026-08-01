#include "devices/switch/gpio/GpioSwitchDeviceConfig.h"

#include "devices/core/ConfigCodec.h"
#include "platform/BoardPinCapabilities.h"

#include <type_traits>

namespace ewfm {

EWFM_LEGACY_CONFIG_USE_BEGIN
static_assert(std::is_trivially_copyable<SwitchDeviceConfigV1>::value, "SwitchDeviceConfigV1 must be POD");
static_assert(std::is_trivially_copyable<GpioSwitchDeviceConfigV1>::value, "GpioSwitchDeviceConfigV1 must be POD");
static_assert(std::is_trivially_copyable<GpioSwitchDevicePersistedConfigV1>::value, "GpioSwitchDevicePersistedConfigV1 must be POD");
static_assert(std::is_trivially_copyable<GpioSwitchDeviceConfigV2>::value, "GpioSwitchDeviceConfigV2 must be POD");
static_assert(sizeof(SwitchDeviceConfigV1) == 38, "SwitchDeviceConfigV1 layout changed");
static_assert(sizeof(GpioSwitchDeviceConfigV1) == 1, "GpioSwitchDeviceConfigV1 layout changed");
static_assert(sizeof(GpioSwitchDeviceConfigV2) == 39, "GpioSwitchDeviceConfigV2 layout changed");
EWFM_LEGACY_CONFIG_USE_END
static_assert(std::is_trivially_copyable<GpioSwitchDeviceConfigV3>::value, "GpioSwitchDeviceConfigV3 must be POD");
static_assert(sizeof(GpioSwitchDeviceConfigV3) == 39, "GpioSwitchDeviceConfigV3 layout changed");
static_assert(sizeof(GpioSwitchDeviceConfigV3::kMagic) - 1U + sizeof(GpioSwitchDeviceConfigV3) <= kMaxDeviceConfigBytes,
              "GpioSwitchDeviceConfigV3 exceeds device config bound");

bool decodeGpioSwitchDeviceConfig(const uint8_t* blob, size_t size, GpioSwitchDeviceConfigV3& config) {
    if (decodeValidatedFixedConfigBlob(GpioSwitchDeviceConfigV3::kMagic, blob, size, config)) {
        return true;
    }

    EWFM_LEGACY_CONFIG_USE_BEGIN
    GpioSwitchDeviceConfigV2 legacyV2{};
    if (decodeFixedConfigBlob(GpioSwitchDeviceConfigV2::kMagic, blob, size, legacyV2)) {
        if (!legacyV2.SwitchDeviceConfigV1::validate().ok() || !gpioSwitchPinIsValid(legacyV2.gpioPin)) {
            return false;
        }
        config.migrateFrom(legacyV2);
        return config.validate().ok();
    }

    GpioSwitchDevicePersistedConfigV1 legacy{};
    size_t pos = 0;
    if (!readFixedConfigSegment(SwitchDeviceConfigV1::kMagic, blob, size, pos, legacy.switchConfig) ||
        !readFixedConfigSegment(GpioSwitchDeviceConfigV1::kMagic, blob, size, pos, legacy.gpioConfig) || pos != size) {
        return false;
    }
    if (!legacy.switchConfig.validate().ok() || !gpioSwitchPinIsValid(legacy.gpioConfig.gpioPin)) {
        return false;
    }
    legacyV2 = {};
    static_cast<SwitchDeviceConfigV1&>(legacyV2) = legacy.switchConfig;
    legacyV2.gpioPin = legacy.gpioConfig.gpioPin;
    config.migrateFrom(legacyV2);
    return config.validate().ok();
    EWFM_LEGACY_CONFIG_USE_END
}

bool GpioSwitchDeviceConfigV3::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!SwitchDeviceConfigV2::parseJson(input, error)) {
        return false;
    }

    gpioPin = static_cast<uint8_t>(input["gpioPin"] | static_cast<int>(gpioPin));
    if (!gpioSwitchPinIsValid(gpioPin)) {
        error = "gpio switch pin is invalid";
        return false;
    }
    return true;
}

DeviceValidationResult GpioSwitchDeviceConfigV3::validate() const {
    const DeviceValidationResult switchResult = SwitchDeviceConfigV2::validate();
    if (!switchResult.ok()) {
        return switchResult;
    }
    return gpioSwitchPinIsValid(gpioPin) ? DeviceValidationResult{}
                                         : DeviceValidationResult{DeviceError::InvalidConfig, "gpio switch pin is invalid"};
}

void GpioSwitchDeviceConfigV3::writeJson(JsonObject output) const {
    SwitchDeviceConfigV2::writeJson(output);
    output["gpioPin"] = gpioPin;
}

EWFM_LEGACY_CONFIG_USE_BEGIN
void GpioSwitchDeviceConfigV3::migrateFrom(const GpioSwitchDeviceConfigV2& legacy) {
    SwitchDeviceConfigV2::migrateFrom(legacy);
    gpioPin = legacy.gpioPin;
}
EWFM_LEGACY_CONFIG_USE_END

bool gpioSwitchPinIsValid(uint8_t pin) {
    return boardPinHasRole(pin, kPinRoleOutput);
}

} // namespace ewfm
