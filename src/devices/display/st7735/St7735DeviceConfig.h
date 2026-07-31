#pragma once

#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

// ST7735 controller panel variants. Each entry pins a fixed native (rotation=0) resolution and
// selects the Adafruit_ST7735::initR() tab option; width/height in the config are derived from
// this, not independently settable. See docs/oled-display-layout.md.
enum class St7735Panel : uint8_t {
    Black18 = 0,       // 1.8" 128x160, black tab (INITR_BLACKTAB) -- previous hardcoded default
    Green18 = 1,       // 1.8" 128x160, green tab (INITR_GREENTAB)
    Green144 = 2,      // 1.44" 128x128 (INITR_144GREENTAB)
    Mini096 = 3,       // 0.96" 80x160 (INITR_MINI160x80)
    Mini096Plugin = 4, // 0.96" 80x160, plugin variant (INITR_MINI160x80_PLUGIN)
};

// Sentinel for resetPin meaning "not wired" -- many ST7735 modules tie RESET to EN/3.3V instead of
// a GPIO. V5 used int8_t{-1} for this; V6 narrows to the project-wide uint8_t/0xFF sentinel, which
// is why this needs a version bump rather than an in-place edit (see docs/pin-configuration-conventions.md).
constexpr uint8_t kSt7735ResetPinUnset = 0xFFU;

#pragma pack(push, 1)
// Legacy persisted layouts (V1-V5): kept only so old blobs can be decoded and migrated to V6.
struct [[deprecated("legacy persisted ST7735 config; decode/migration only")]] St7735DeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "STV1";
    uint32_t spiBusDeviceId{0};
    uint8_t chipSelectPin{5};
    uint16_t layoutWidth{128};
    uint16_t layoutHeight{160};
};

struct [[deprecated("legacy persisted ST7735 config; decode/migration only")]] St7735DeviceConfigV2 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "STV2";
    uint32_t spiBusDeviceId{0};
    uint8_t chipSelectPin{5};
    uint8_t dcPin{2};
    int8_t resetPin{-1};
    uint16_t layoutWidth{128};
    uint16_t layoutHeight{160};
};

struct [[deprecated("legacy persisted ST7735 config; decode/migration only")]] St7735DeviceConfigV3 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "STV3";
    uint32_t spiBusDeviceId{0};
    uint8_t chipSelectPin{5};
    uint8_t dcPin{2};
    int8_t resetPin{-1};
    uint16_t width{128};
    uint16_t height{160};
};

struct [[deprecated("legacy persisted ST7735 config; decode/migration only")]] St7735DeviceConfigV4 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "STV4";
    uint32_t spiBusDeviceId{0};
    uint8_t chipSelectPin{5};
    uint8_t dcPin{2};
    int8_t resetPin{-1};
    uint8_t rotation{0};
    uint16_t width{128};
    uint16_t height{160};

    DeviceValidationResult validate() const;
};

struct [[deprecated("legacy persisted ST7735 config; decode/migration only")]] St7735DeviceConfigV5 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "STV5";
    uint32_t spiBusDeviceId{0};
    uint8_t chipSelectPin{5};
    uint8_t dcPin{2};
    int8_t resetPin{-1};
    uint8_t rotation{0};
    uint8_t panel{static_cast<uint8_t>(St7735Panel::Black18)};
    uint16_t width{128};
    uint16_t height{160};

    DeviceValidationResult validate() const;
};

struct St7735DeviceConfigV6 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "STV6";
    uint32_t spiBusDeviceId{0};
    uint8_t chipSelectPin{5};
    uint8_t dcPin{2};
    uint8_t resetPin{kSt7735ResetPinUnset};
    uint8_t rotation{0};
    uint8_t panel{static_cast<uint8_t>(St7735Panel::Black18)};
    uint16_t width{128};
    uint16_t height{160};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const St7735DeviceConfigV1& origState);
    void migrateFrom(const St7735DeviceConfigV2& origState);
    void migrateFrom(const St7735DeviceConfigV3& origState);
    void migrateFrom(const St7735DeviceConfigV4& origState);
    void migrateFrom(const St7735DeviceConfigV5& origState);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t st7735DeviceConfigV1Size() {
    return sizeof(St7735DeviceConfigV1::kMagic) - 1U + sizeof(St7735DeviceConfigV1);
}

constexpr size_t st7735DeviceConfigV2Size() {
    return sizeof(St7735DeviceConfigV2::kMagic) - 1U + sizeof(St7735DeviceConfigV2);
}

constexpr size_t st7735DeviceConfigSize(const St7735DeviceConfigV3&) {
    return sizeof(St7735DeviceConfigV3::kMagic) - 1U + sizeof(St7735DeviceConfigV3);
}

constexpr size_t st7735DeviceConfigSize(const St7735DeviceConfigV4&) {
    return sizeof(St7735DeviceConfigV4::kMagic) - 1U + sizeof(St7735DeviceConfigV4);
}

constexpr size_t st7735DeviceConfigSize(const St7735DeviceConfigV5&) {
    return sizeof(St7735DeviceConfigV5::kMagic) - 1U + sizeof(St7735DeviceConfigV5);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t st7735DeviceConfigSize(const St7735DeviceConfigV6&) {
    return sizeof(St7735DeviceConfigV6::kMagic) - 1U + sizeof(St7735DeviceConfigV6);
}

bool st7735PanelFromString(const char* value, St7735Panel& panel);
const char* st7735PanelName(St7735Panel panel);
// Native (rotation=0) resolution for the panel, per the Adafruit_ST7735 initR() tab it selects.
void st7735PanelGeometry(St7735Panel panel, uint16_t& width, uint16_t& height);

bool decodeSt7735DeviceConfig(const uint8_t* blob, size_t size, St7735DeviceConfigV6& config);

} // namespace ewfm
