#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/switch/OutputState.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct SwitchDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "SWCFG1";
    bool restorePreviousState{false};
    OutputState startupState{OutputState::Off};
    OutputState safeState{OutputState::Off};
    bool inverted{false};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t switchDeviceConfigSize(const SwitchDeviceConfigV1&) {
    return sizeof(SwitchDeviceConfigV1::kMagic) - 1U + sizeof(SwitchDeviceConfigV1);
}

bool encodeSwitchDeviceConfig(const SwitchDeviceConfigV1& config, uint8_t* blob, size_t capacity);
bool decodeSwitchDeviceConfig(const uint8_t* blob, size_t size, SwitchDeviceConfigV1& config);
DeviceValidationResult validateSwitchDeviceConfig(const SwitchDeviceConfigV1& config);
bool switchConfigStartupState(const SwitchDeviceConfigV1& config, OutputState& state);
bool switchConfigSafeState(const SwitchDeviceConfigV1& config, OutputState& state);

} // namespace ewfm
