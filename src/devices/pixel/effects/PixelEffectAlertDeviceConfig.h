#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kPixelEffectAlertDeviceTypeId = 38;
constexpr uint32_t kPixelEffectAlertDeviceConfigVersion = 1;
constexpr uint32_t kPixelEffectAlertDefaultBlinkIntervalMs = 500;
constexpr uint32_t kPixelEffectAlertMinBlinkIntervalMs = 100;
constexpr uint32_t kPixelEffectAlertMaxBlinkIntervalMs = 60000;
// Bounds how many Condition-role AND dependencies a PixelEffectAlertDevice can have, in addition
// to its one required target PixelStrip dependency -- mirrors AutoSwitchDevice's
// kMaxAutoSwitchConditions. Conditions live in `deps`, not in this config blob (see
// device-model-structures.md's "dependency links are persisted by the registry, not inside the
// config" rule), the same way AutoSwitchDeviceConfigV1 carries no condition list of its own.
constexpr size_t kMaxPixelEffectAlertConditions = 4;

#pragma pack(push, 1)
struct PixelEffectAlertDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "PIXELFXALERT-1";
    PixelColor color{};
    uint32_t blinkIntervalMs{kPixelEffectAlertDefaultBlinkIntervalMs};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t pixelEffectAlertDeviceConfigSize(const PixelEffectAlertDeviceConfigV1&) {
    return sizeof(PixelEffectAlertDeviceConfigV1::kMagic) - 1U + sizeof(PixelEffectAlertDeviceConfigV1);
}

bool decodePixelEffectAlertDeviceConfig(const uint8_t* blob, size_t size, PixelEffectAlertDeviceConfigV1& config);

} // namespace ewfm
