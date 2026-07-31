#pragma once

#include "devices/switch/SwitchDeviceConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
// Legacy persisted layouts: kept only so old blobs can be decoded and migrated to V3.
struct [[deprecated("legacy persisted gpio-switch config; decode/migration only")]] GpioSwitchDeviceConfigV1 {
    static constexpr char kMagic[] = "GSW-CHILD-1";
    uint8_t gpioPin{2};
};

struct [[deprecated("legacy persisted gpio-switch config; decode/migration only")]] GpioSwitchDevicePersistedConfigV1 {
    EWFM_LEGACY_CONFIG_USE_BEGIN
    SwitchDeviceConfigV1 switchConfig{};
    GpioSwitchDeviceConfigV1 gpioConfig{};
    EWFM_LEGACY_CONFIG_USE_END
};

EWFM_LEGACY_CONFIG_USE_BEGIN
struct [[deprecated("legacy persisted gpio-switch config; decode/migration only")]] GpioSwitchDeviceConfigV2 : SwitchDeviceConfigV1 {
    static constexpr char kMagic[] = "GSW2";
    uint8_t gpioPin{2};
};
EWFM_LEGACY_CONFIG_USE_END

struct GpioSwitchDeviceConfigV3 : SwitchDeviceConfigV2 {
    static constexpr char kMagic[] = "GSW3";
    // No real default GPIO exists for a plain switch; 0xFF forces the user to pick one explicitly
    // (gpioSwitchPinIsValid() rejects it) rather than silently defaulting to some real pin. See
    // docs/pin-configuration-conventions.md.
    uint8_t gpioPin{0xFFU};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const GpioSwitchDeviceConfigV2& legacy);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t gpioSwitchDeviceConfigSize(const GpioSwitchDevicePersistedConfigV1&) {
    return sizeof(SwitchDeviceConfigV1::kMagic) - 1U + sizeof(SwitchDeviceConfigV1) + sizeof(GpioSwitchDeviceConfigV1::kMagic) - 1U +
           sizeof(GpioSwitchDeviceConfigV1);
}

constexpr size_t gpioSwitchDeviceConfigSize(const GpioSwitchDeviceConfigV2&) {
    return sizeof(GpioSwitchDeviceConfigV2::kMagic) - 1U + sizeof(GpioSwitchDeviceConfigV2);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t gpioSwitchDeviceConfigSize(const GpioSwitchDeviceConfigV3&) {
    return sizeof(GpioSwitchDeviceConfigV3::kMagic) - 1U + sizeof(GpioSwitchDeviceConfigV3);
}

bool decodeGpioSwitchDeviceConfig(const uint8_t* blob, size_t size, GpioSwitchDeviceConfigV3& config);
bool gpioSwitchPinIsValid(uint8_t pin);

} // namespace ewfm
