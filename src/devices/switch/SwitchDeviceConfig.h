#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/switch/OutputState.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct SwitchDeviceConfigV1 {
    static constexpr char kMagic[] = "SWCFG1";
    DeviceBaseConfigV1 base{};
    uint8_t restorePreviousState{0};
    uint8_t startupState{static_cast<uint8_t>(OutputState::Off)};
    uint8_t safeState{static_cast<uint8_t>(OutputState::Off)};
    uint8_t inverted{0};
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
