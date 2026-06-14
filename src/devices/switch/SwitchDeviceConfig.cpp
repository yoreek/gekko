#include "devices/switch/SwitchDeviceConfig.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<SwitchDeviceConfigV1>::value, "SwitchDeviceConfigV1 must be POD");
static_assert(sizeof(SwitchDeviceConfigV1) == 5, "SwitchDeviceConfigV1 layout changed");
static_assert(sizeof(SwitchDeviceConfigV1::kMagicKey) + sizeof(SwitchDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "SwitchDeviceConfigV1 exceeds device config bound");

std::string encodeSwitchDeviceConfig(const SwitchDeviceConfigV1& config) {
    std::string blob;
    blob.resize(sizeof(SwitchDeviceConfigV1::kMagicKey) + sizeof(SwitchDeviceConfigV1));
    std::memcpy(blob.data(), &SwitchDeviceConfigV1::kMagicKey, sizeof(SwitchDeviceConfigV1::kMagicKey));
    std::memcpy(blob.data() + sizeof(SwitchDeviceConfigV1::kMagicKey), &config, sizeof(SwitchDeviceConfigV1));
    return blob;
}

bool decodeSwitchDeviceConfig(const std::string& blob, SwitchDeviceConfigV1& config) {
    constexpr size_t kBlobSize = sizeof(SwitchDeviceConfigV1::kMagicKey) + sizeof(SwitchDeviceConfigV1);
    if (blob.size() != kBlobSize) {
        return false;
    }

    uint32_t magicKey{0};
    std::memcpy(&magicKey, blob.data(), sizeof(magicKey));
    if (magicKey != SwitchDeviceConfigV1::kMagicKey) {
        return false;
    }

    std::memcpy(&config, blob.data() + sizeof(magicKey), sizeof(SwitchDeviceConfigV1));
    OutputState state{};
    return outputStateFromByte(config.startupState, state) && outputStateFromByte(config.safeState, state);
}

bool switchConfigStartupState(const SwitchDeviceConfigV1& config, OutputState& state) {
    return outputStateFromByte(config.startupState, state);
}

bool switchConfigSafeState(const SwitchDeviceConfigV1& config, OutputState& state) {
    return outputStateFromByte(config.safeState, state);
}

} // namespace ewfm
