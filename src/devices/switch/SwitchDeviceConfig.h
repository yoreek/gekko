#pragma once

#include "devices/output/OutputDeviceConfig.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

template <> struct OutputDeviceValueCodec<bool> {
    static bool parseJson(const JsonVariantConst& input, bool& state, const char*& error);
    static bool valid(bool state);
    static void writeJson(JsonObject output, const char* key, bool state);
};

#pragma pack(push, 1)
struct SwitchDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "SWCFG1";
    bool restorePreviousState{false};
    uint8_t startupState{0U};
    uint8_t safeState{0U};
    bool inverted{false};

    DeviceValidationResult validate() const;
};

struct SwitchDeviceConfigV2 : OutputDeviceConfigV1<bool> {
    static constexpr char kMagic[] = "SWCFG2";

    void migrateFrom(const SwitchDeviceConfigV1& legacy);
};
#pragma pack(pop)

constexpr size_t switchDeviceConfigSize(const SwitchDeviceConfigV1&) {
    return sizeof(SwitchDeviceConfigV1::kMagic) - 1U + sizeof(SwitchDeviceConfigV1);
}

constexpr size_t switchDeviceConfigSize(const SwitchDeviceConfigV2&) {
    return sizeof(SwitchDeviceConfigV2::kMagic) - 1U + sizeof(SwitchDeviceConfigV2);
}

} // namespace ewfm
