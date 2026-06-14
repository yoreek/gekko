#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/switch/OutputState.h"

#include <string>

namespace ewfm {

#pragma pack(push, 1)
struct SwitchDeviceConfigV1 {
    static constexpr uint32_t kMagicKey = 0x53574331UL;
    uint8_t enabled{1};
    uint8_t restorePreviousState{0};
    uint8_t startupState{static_cast<uint8_t>(OutputState::Off)};
    uint8_t safeState{static_cast<uint8_t>(OutputState::Off)};
    uint8_t inverted{0};
};
#pragma pack(pop)

std::string encodeSwitchDeviceConfig(const SwitchDeviceConfigV1& config);
bool decodeSwitchDeviceConfig(const std::string& blob, SwitchDeviceConfigV1& config);
bool switchConfigStartupState(const SwitchDeviceConfigV1& config, OutputState& state);
bool switchConfigSafeState(const SwitchDeviceConfigV1& config, OutputState& state);

} // namespace ewfm
