#pragma once

#include "devices/switch/SwitchDeviceConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

// Any PCF857x variant tops out at 16 channels (PCF8575); used as a cheap, dependency-independent
// sanity bound. The real bound (the attached expander's actual channelCount()) is enforced by the
// REST adapter, which has registry access to the dependency.
constexpr uint8_t kMaxPortExpanderChannel = 15;

#pragma pack(push, 1)
// Legacy persisted layouts: kept only so old blobs can be decoded and migrated to V3.
struct [[deprecated("legacy persisted port-expander-switch config; decode/migration only")]] PortExpanderSwitchDeviceConfigV1 {
    static constexpr char kMagic[] = "PXSW-CHILD-1";
    uint8_t channel{0};
};

struct [[deprecated("legacy persisted port-expander-switch config; decode/migration only")]] PortExpanderSwitchDevicePersistedConfigV1 {
    EWFM_LEGACY_CONFIG_USE_BEGIN
    SwitchDeviceConfigV1 switchConfig{};
    PortExpanderSwitchDeviceConfigV1 expanderConfig{};
    EWFM_LEGACY_CONFIG_USE_END
};

EWFM_LEGACY_CONFIG_USE_BEGIN
struct [[deprecated("legacy persisted port-expander-switch config; decode/migration only")]] PortExpanderSwitchDeviceConfigV2
    : SwitchDeviceConfigV1 {
    static constexpr char kMagic[] = "PXSW2";
    uint8_t channel{0};
};
EWFM_LEGACY_CONFIG_USE_END

struct PortExpanderSwitchDeviceConfigV3 : SwitchDeviceConfigV2 {
    static constexpr char kMagic[] = "PXSW3";
    uint8_t channel{0};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const PortExpanderSwitchDeviceConfigV2& legacy);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t portExpanderSwitchDeviceConfigSize(const PortExpanderSwitchDevicePersistedConfigV1&) {
    return sizeof(SwitchDeviceConfigV1::kMagic) - 1U + sizeof(SwitchDeviceConfigV1) + sizeof(PortExpanderSwitchDeviceConfigV1::kMagic) -
           1U + sizeof(PortExpanderSwitchDeviceConfigV1);
}

constexpr size_t portExpanderSwitchDeviceConfigSize(const PortExpanderSwitchDeviceConfigV2&) {
    return sizeof(PortExpanderSwitchDeviceConfigV2::kMagic) - 1U + sizeof(PortExpanderSwitchDeviceConfigV2);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t portExpanderSwitchDeviceConfigSize(const PortExpanderSwitchDeviceConfigV3&) {
    return sizeof(PortExpanderSwitchDeviceConfigV3::kMagic) - 1U + sizeof(PortExpanderSwitchDeviceConfigV3);
}

bool decodePortExpanderSwitchDeviceConfig(const uint8_t* blob, size_t size, PortExpanderSwitchDeviceConfigV3& config);
bool portExpanderSwitchChannelIsValid(uint8_t channel);

} // namespace ewfm
