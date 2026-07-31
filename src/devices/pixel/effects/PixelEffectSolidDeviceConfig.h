#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kPixelEffectSolidDeviceTypeId = 37;
constexpr uint32_t kPixelEffectSolidDeviceConfigVersion = 1;

#pragma pack(push, 1)
struct PixelEffectSolidDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "PIXELFXSOLID-1";
    // Whether to power up at the last live color (retained state, see PixelEffectSolidDevice) or
    // always at startupColor -- mirrors OutputDeviceConfigV1::restorePreviousState.
    bool restorePreviousState{false};
    // Applied only at startup (or when no retained state is available); the live, currently-shown
    // color is runtime state set via the SetOutput command, never persisted config -- mirrors
    // analog_output's startupState/currentOutputState split (docs/analog-output.md).
    PixelColor startupColor{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t pixelEffectSolidDeviceConfigSize(const PixelEffectSolidDeviceConfigV1&) {
    return sizeof(PixelEffectSolidDeviceConfigV1::kMagic) - 1U + sizeof(PixelEffectSolidDeviceConfigV1);
}

bool decodePixelEffectSolidDeviceConfig(const uint8_t* blob, size_t size, PixelEffectSolidDeviceConfigV1& config);

} // namespace ewfm
