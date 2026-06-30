#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct St7735DeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "STV1";
    uint32_t spiBusDeviceId{0};
    uint8_t chipSelectPin{5};
    uint16_t layoutWidth{128};
    uint16_t layoutHeight{160};
};

struct St7735DeviceConfigV2 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "STV2";
    uint32_t spiBusDeviceId{0};
    uint8_t chipSelectPin{5};
    uint8_t dcPin{2};
    int8_t resetPin{-1};
    uint16_t layoutWidth{128};
    uint16_t layoutHeight{160};
};

struct St7735DeviceConfigV3 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "STV3";
    uint32_t spiBusDeviceId{0};
    uint8_t chipSelectPin{5};
    uint8_t dcPin{2};
    int8_t resetPin{-1};
    uint16_t width{128};
    uint16_t height{160};
};

struct St7735DeviceConfigV4 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "STV4";
    uint32_t spiBusDeviceId{0};
    uint8_t chipSelectPin{5};
    uint8_t dcPin{2};
    int8_t resetPin{-1};
    uint8_t rotation{0};
    uint16_t width{128};
    uint16_t height{160};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    void migrateFrom(const St7735DeviceConfigV1& origState);
    void migrateFrom(const St7735DeviceConfigV2& origState);
    void migrateFrom(const St7735DeviceConfigV3& origState);
};
#pragma pack(pop)

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

bool encodeSt7735DeviceConfig(const St7735DeviceConfigV4& config, uint8_t* blob, size_t capacity);
bool decodeSt7735DeviceConfig(const uint8_t* blob, size_t size, St7735DeviceConfigV4& config);

} // namespace ewfm
