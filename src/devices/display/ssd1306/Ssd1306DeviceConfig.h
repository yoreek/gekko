#pragma once

#include "devices/bus/i2c/I2cDeviceConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

// SSD1306 panel size presets. Each entry pins a fixed width/height, matching the resolutions
// Adafruit_SSD1306::begin() has tuned comPins/contrast values for. `Custom` allows any width/height
// (for less common panel variants the driver falls back to generic defaults for). See
// docs/oled-display-layout.md.
enum class Ssd1306Panel : uint8_t {
    Preset128x64 = 0, // default
    Preset128x32 = 1,
    Preset96x16 = 2,
    Preset64x32 = 3,
    Custom = 4,
};

#pragma pack(push, 1)
// Legacy persisted layouts (V1-V5): kept only so old blobs can be decoded and migrated to V6.
struct [[deprecated("legacy persisted SSD1306 config; decode/migration only")]] Ssd1306DeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "OLV1";
    uint32_t i2cBusDeviceId{0};
    uint8_t i2cAddress{0x3C};
    uint16_t layoutWidth{128};
    uint16_t layoutHeight{64};
};

struct [[deprecated("legacy persisted SSD1306 config; decode/migration only")]] Ssd1306DeviceConfigV2 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "OLV2";
    uint32_t i2cBusDeviceId{0};
    uint8_t i2cAddress{0x3C};
    uint16_t width{128};
    uint16_t height{64};
};

struct [[deprecated("legacy persisted SSD1306 config; decode/migration only")]] Ssd1306DeviceConfigV3 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "OLV3";
    uint32_t i2cBusDeviceId{0};
    uint8_t i2cAddress{0x3C};
    uint8_t rotation{0};
    uint16_t width{128};
    uint16_t height{64};
};

struct [[deprecated("legacy persisted SSD1306 config; decode/migration only")]] Ssd1306DeviceConfigV4 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "OLV4";
    uint8_t i2cAddress{0x3C};
    uint8_t rotation{0};
    uint16_t width{128};
    uint16_t height{64};

    DeviceValidationResult validate() const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const Ssd1306DeviceConfigV1& origState);
    void migrateFrom(const Ssd1306DeviceConfigV2& origState);
    void migrateFrom(const Ssd1306DeviceConfigV3& origState);
    EWFM_LEGACY_CONFIG_USE_END
};

struct [[deprecated("legacy persisted SSD1306 config; decode/migration only")]] Ssd1306DeviceConfigV5 : I2cDeviceConfigV1 {
    static constexpr char kMagic[] = "OLV5";

    Ssd1306DeviceConfigV5() : I2cDeviceConfigV1(0x3CU) {}

    uint8_t rotation{0};
    uint16_t width{128};
    uint16_t height{64};

    DeviceValidationResult validate() const;
};

struct Ssd1306DeviceConfigV6 : I2cDeviceConfigV1 {
    static constexpr char kMagic[] = "OLV6";

    Ssd1306DeviceConfigV6() : I2cDeviceConfigV1(0x3CU) {}

    uint8_t rotation{0};
    uint8_t panel{static_cast<uint8_t>(Ssd1306Panel::Preset128x64)};
    uint16_t width{128};
    uint16_t height{64};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    EWFM_LEGACY_CONFIG_USE_BEGIN
    void migrateFrom(const Ssd1306DeviceConfigV1& origState);
    void migrateFrom(const Ssd1306DeviceConfigV2& origState);
    void migrateFrom(const Ssd1306DeviceConfigV3& origState);
    void migrateFrom(const Ssd1306DeviceConfigV4& origState);
    void migrateFrom(const Ssd1306DeviceConfigV5& origState);
    EWFM_LEGACY_CONFIG_USE_END
};
#pragma pack(pop)

EWFM_LEGACY_CONFIG_USE_BEGIN
constexpr size_t ssd1306DeviceConfigV1Size() {
    return sizeof(Ssd1306DeviceConfigV1::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV1);
}

constexpr size_t ssd1306DeviceConfigSize(const Ssd1306DeviceConfigV2&) {
    return sizeof(Ssd1306DeviceConfigV2::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV2);
}

constexpr size_t ssd1306DeviceConfigSize(const Ssd1306DeviceConfigV3&) {
    return sizeof(Ssd1306DeviceConfigV3::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV3);
}

constexpr size_t ssd1306DeviceConfigSize(const Ssd1306DeviceConfigV4&) {
    return sizeof(Ssd1306DeviceConfigV4::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV4);
}

constexpr size_t ssd1306DeviceConfigSize(const Ssd1306DeviceConfigV5&) {
    return sizeof(Ssd1306DeviceConfigV5::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV5);
}
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t ssd1306DeviceConfigSize(const Ssd1306DeviceConfigV6&) {
    return sizeof(Ssd1306DeviceConfigV6::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV6);
}

bool ssd1306PanelFromString(const char* value, Ssd1306Panel& panel);
const char* ssd1306PanelName(Ssd1306Panel panel);
// Native resolution for the preset; returns false for Custom (no fixed geometry).
bool ssd1306PanelGeometry(Ssd1306Panel panel, uint16_t& width, uint16_t& height);

bool decodeSsd1306DeviceConfig(const uint8_t* blob, size_t size, Ssd1306DeviceConfigV6& config);

} // namespace ewfm
