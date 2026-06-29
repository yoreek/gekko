#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct Ssd1306DeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "OLV1";
    uint32_t i2cBusDeviceId{0};
    uint8_t i2cAddress{0x3C};
    uint16_t layoutWidth{128};
    uint16_t layoutHeight{64};
};

struct Ssd1306DeviceConfigV2 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "OLV2";
    uint32_t i2cBusDeviceId{0};
    uint8_t i2cAddress{0x3C};
    uint16_t width{128};
    uint16_t height{64};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    void migrateFrom(const Ssd1306DeviceConfigV1& origState);
};
#pragma pack(pop)

constexpr size_t ssd1306DeviceConfigV1Size() {
    return sizeof(Ssd1306DeviceConfigV1::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV1);
}

constexpr size_t ssd1306DeviceConfigSize(const Ssd1306DeviceConfigV2&) {
    return sizeof(Ssd1306DeviceConfigV2::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV2);
}

bool encodeSsd1306DeviceConfig(const Ssd1306DeviceConfigV2& config, uint8_t* blob, size_t capacity);
bool decodeSsd1306DeviceConfig(const uint8_t* blob, size_t size, Ssd1306DeviceConfigV2& config);

} // namespace ewfm
