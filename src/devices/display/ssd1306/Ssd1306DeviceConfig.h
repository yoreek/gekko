#pragma once

#include "devices/bus/i2c/I2cDeviceConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
// Legacy persisted layouts (V1-V4): kept only so old blobs can be decoded and migrated to V5.
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

struct Ssd1306DeviceConfigV5 : I2cDeviceConfigV1 {
    static constexpr char kMagic[] = "OLV5";

    Ssd1306DeviceConfigV5() : I2cDeviceConfigV1(0x3CU) {}

    uint8_t rotation{0};
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
EWFM_LEGACY_CONFIG_USE_END

constexpr size_t ssd1306DeviceConfigSize(const Ssd1306DeviceConfigV5&) {
    return sizeof(Ssd1306DeviceConfigV5::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV5);
}

bool decodeSsd1306DeviceConfig(const uint8_t* blob, size_t size, Ssd1306DeviceConfigV5& config);

} // namespace ewfm
