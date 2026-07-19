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
struct PortExpanderSwitchDeviceConfigV1 {
    static constexpr char kMagic[] = "PXSW-CHILD-1";
    uint8_t channel{0};
};

struct PortExpanderSwitchDevicePersistedConfigV1 {
    SwitchDeviceConfigV1 switchConfig{};
    PortExpanderSwitchDeviceConfigV1 expanderConfig{};
};

struct PortExpanderSwitchDeviceConfigV2 : SwitchDeviceConfigV1 {
    static constexpr char kMagic[] = "PXSW2";
    uint8_t channel{0};
};

struct PortExpanderSwitchDeviceConfigV3 : SwitchDeviceConfigV2 {
    static constexpr char kMagic[] = "PXSW3";
    uint8_t channel{0};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    void migrateFrom(const PortExpanderSwitchDeviceConfigV2& legacy);
};
#pragma pack(pop)

constexpr size_t portExpanderSwitchDeviceConfigSize(const PortExpanderSwitchDevicePersistedConfigV1&) {
    return sizeof(SwitchDeviceConfigV1::kMagic) - 1U + sizeof(SwitchDeviceConfigV1) + sizeof(PortExpanderSwitchDeviceConfigV1::kMagic) -
           1U + sizeof(PortExpanderSwitchDeviceConfigV1);
}

constexpr size_t portExpanderSwitchDeviceConfigSize(const PortExpanderSwitchDeviceConfigV2&) {
    return sizeof(PortExpanderSwitchDeviceConfigV2::kMagic) - 1U + sizeof(PortExpanderSwitchDeviceConfigV2);
}

constexpr size_t portExpanderSwitchDeviceConfigSize(const PortExpanderSwitchDeviceConfigV3&) {
    return sizeof(PortExpanderSwitchDeviceConfigV3::kMagic) - 1U + sizeof(PortExpanderSwitchDeviceConfigV3);
}

bool decodePortExpanderSwitchDeviceConfig(const uint8_t* blob, size_t size, PortExpanderSwitchDeviceConfigV3& config);
bool portExpanderSwitchChannelIsValid(uint8_t channel);

} // namespace ewfm
