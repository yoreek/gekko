#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

struct St7735LegacyDeviceConfigV1;

#pragma pack(push, 1)
struct St7735DeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "STV2";
    static constexpr char kLegacyMagic[] = "STV1";
    uint32_t spiBusDeviceId{0};
    uint8_t chipSelectPin{5};
    uint8_t dcPin{2};
    int8_t resetPin{-1};
    uint16_t layoutWidth{128};
    uint16_t layoutHeight{160};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
    void migrateFrom(const St7735LegacyDeviceConfigV1& origState);
};
#pragma pack(pop)

constexpr size_t st7735DeviceConfigSize(const St7735DeviceConfigV1&) {
    return sizeof(St7735DeviceConfigV1::kMagic) - 1U + sizeof(St7735DeviceConfigV1);
}

constexpr size_t st7735LegacyDeviceConfigSize() {
    return sizeof(St7735DeviceConfigV1::kLegacyMagic) - 1U + sizeof(DeviceBaseConfigV1) + sizeof(uint32_t) + sizeof(uint8_t) +
           sizeof(uint16_t) + sizeof(uint16_t);
}

bool encodeSt7735DeviceConfig(const St7735DeviceConfigV1& config, uint8_t* blob, size_t capacity);
bool decodeSt7735DeviceConfig(const uint8_t* blob, size_t size, St7735DeviceConfigV1& config);

} // namespace ewfm
