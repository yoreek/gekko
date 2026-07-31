#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kPixelStripDeviceTypeId = 36;
constexpr uint32_t kPixelStripDeviceConfigVersion = 1;
constexpr uint8_t kPixelBrightnessMax = 255U;

// brightness is stored internally as the raw 0..255 scale Adafruit_NeoPixel::setBrightness()
// expects, but the REST/SPA boundary only ever sees a 0..100 percent value -- mirrors
// docs/analog-output.md's OutputDeviceValueCodec<uint16_t> pattern (internal units in the config
// struct, percent at the wire boundary), scaled down to a plain pair of free functions since
// pixel_strip is a new sibling family, not part of the analog_output family these live in.
constexpr uint8_t percentToPixelBrightness(uint8_t percent) {
    return static_cast<uint8_t>((static_cast<uint16_t>(percent) * static_cast<uint16_t>(kPixelBrightnessMax) + 50U) / 100U);
}

constexpr uint8_t pixelBrightnessToPercent(uint8_t brightness) {
    return static_cast<uint8_t>((static_cast<uint16_t>(brightness) * 100U + (kPixelBrightnessMax / 2U)) / kPixelBrightnessMax);
}

#pragma pack(push, 1)
struct PixelStripDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "PIXELSTRIP-1";
    uint8_t pin{0xFFU};
    uint16_t pixelCount{1U};
    // Whether to power up at the last live brightness (retained state, see PixelStripDevice) or
    // always at startupBrightness -- mirrors OutputDeviceConfigV1::restorePreviousState.
    bool restorePreviousState{false};
    // Raw 0..255 NeoPixel brightness scale applied only at startup (or when no retained state is
    // available); the live, currently-applied brightness is runtime state set via the SetOutput
    // command, never persisted config -- mirrors analog_output's startupState/currentOutputState
    // split (docs/analog-output.md). See percentToPixelBrightness/pixelBrightnessToPercent above
    // for the JSON-boundary conversion.
    uint8_t startupBrightness{128U};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t pixelStripDeviceConfigSize(const PixelStripDeviceConfigV1&) {
    return sizeof(PixelStripDeviceConfigV1::kMagic) - 1U + sizeof(PixelStripDeviceConfigV1);
}

bool decodePixelStripDeviceConfig(const uint8_t* blob, size_t size, PixelStripDeviceConfigV1& config);

} // namespace ewfm
