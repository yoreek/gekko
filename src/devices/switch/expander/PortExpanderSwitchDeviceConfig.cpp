#include "devices/switch/expander/PortExpanderSwitchDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <type_traits>

namespace ewfm {

EWFM_LEGACY_CONFIG_USE_BEGIN
static_assert(std::is_trivially_copyable<PortExpanderSwitchDeviceConfigV1>::value, "PortExpanderSwitchDeviceConfigV1 must be POD");
static_assert(std::is_trivially_copyable<PortExpanderSwitchDevicePersistedConfigV1>::value,
              "PortExpanderSwitchDevicePersistedConfigV1 must be POD");
static_assert(std::is_trivially_copyable<PortExpanderSwitchDeviceConfigV2>::value, "PortExpanderSwitchDeviceConfigV2 must be POD");
static_assert(sizeof(SwitchDeviceConfigV1) == 38, "SwitchDeviceConfigV1 layout changed");
static_assert(sizeof(PortExpanderSwitchDeviceConfigV1) == 1, "PortExpanderSwitchDeviceConfigV1 layout changed");
static_assert(sizeof(PortExpanderSwitchDeviceConfigV2) == 39, "PortExpanderSwitchDeviceConfigV2 layout changed");
EWFM_LEGACY_CONFIG_USE_END
static_assert(std::is_trivially_copyable<PortExpanderSwitchDeviceConfigV3>::value, "PortExpanderSwitchDeviceConfigV3 must be POD");
static_assert(sizeof(PortExpanderSwitchDeviceConfigV3) == 39, "PortExpanderSwitchDeviceConfigV3 layout changed");
static_assert(sizeof(PortExpanderSwitchDeviceConfigV3::kMagic) - 1U + sizeof(PortExpanderSwitchDeviceConfigV3) <= kMaxDeviceConfigBytes,
              "PortExpanderSwitchDeviceConfigV3 exceeds device config bound");

bool decodePortExpanderSwitchDeviceConfig(const uint8_t* blob, size_t size, PortExpanderSwitchDeviceConfigV3& config) {
    if (decodeValidatedFixedConfigBlob(PortExpanderSwitchDeviceConfigV3::kMagic, blob, size, config)) {
        return true;
    }

    EWFM_LEGACY_CONFIG_USE_BEGIN
    PortExpanderSwitchDeviceConfigV2 legacyV2{};
    if (decodeFixedConfigBlob(PortExpanderSwitchDeviceConfigV2::kMagic, blob, size, legacyV2)) {
        if (!legacyV2.SwitchDeviceConfigV1::validate().ok() || !portExpanderSwitchChannelIsValid(legacyV2.channel)) {
            return false;
        }
        config.migrateFrom(legacyV2);
        return config.validate().ok();
    }

    PortExpanderSwitchDevicePersistedConfigV1 legacy{};
    size_t pos = 0;
    if (!readFixedConfigSegment(SwitchDeviceConfigV1::kMagic, blob, size, pos, legacy.switchConfig) ||
        !readFixedConfigSegment(PortExpanderSwitchDeviceConfigV1::kMagic, blob, size, pos, legacy.expanderConfig) || pos != size) {
        return false;
    }
    if (!legacy.switchConfig.validate().ok() || !portExpanderSwitchChannelIsValid(legacy.expanderConfig.channel)) {
        return false;
    }
    legacyV2 = {};
    static_cast<SwitchDeviceConfigV1&>(legacyV2) = legacy.switchConfig;
    legacyV2.channel = legacy.expanderConfig.channel;
    config.migrateFrom(legacyV2);
    return config.validate().ok();
    EWFM_LEGACY_CONFIG_USE_END
}

bool PortExpanderSwitchDeviceConfigV3::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!SwitchDeviceConfigV2::parseJson(input, error)) {
        return false;
    }

    channel = static_cast<uint8_t>(input["channel"] | static_cast<int>(channel));
    if (!portExpanderSwitchChannelIsValid(channel)) {
        error = "port expander switch channel is invalid";
        return false;
    }
    return true;
}

DeviceValidationResult PortExpanderSwitchDeviceConfigV3::validate() const {
    const DeviceValidationResult switchResult = SwitchDeviceConfigV2::validate();
    if (!switchResult.ok()) {
        return switchResult;
    }
    return portExpanderSwitchChannelIsValid(channel)
               ? DeviceValidationResult{}
               : DeviceValidationResult{DeviceError::InvalidConfig, "port expander switch channel is invalid"};
}

void PortExpanderSwitchDeviceConfigV3::writeJson(JsonObject output) const {
    SwitchDeviceConfigV2::writeJson(output);
    output["channel"] = channel;
}

EWFM_LEGACY_CONFIG_USE_BEGIN
void PortExpanderSwitchDeviceConfigV3::migrateFrom(const PortExpanderSwitchDeviceConfigV2& legacy) {
    SwitchDeviceConfigV2::migrateFrom(legacy);
    channel = legacy.channel;
}
EWFM_LEGACY_CONFIG_USE_END

bool portExpanderSwitchChannelIsValid(uint8_t channel) {
    return channel <= kMaxPortExpanderChannel;
}

} // namespace ewfm
