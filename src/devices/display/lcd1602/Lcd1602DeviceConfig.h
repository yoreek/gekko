#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/hd44780/Hd44780ChannelConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr size_t kLcd1602LineLength = 16U; // HD44780 1602 panel: 16 columns.

#pragma pack(push, 1)
struct Lcd1602DeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "LCD1602-1";

    Hd44780ChannelConfigV1 channels{};
    char line1[kLcd1602LineLength + 1U]{};
    char line2[kLcd1602LineLength + 1U]{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t lcd1602DeviceConfigSize(const Lcd1602DeviceConfigV1&) {
    return sizeof(Lcd1602DeviceConfigV1::kMagic) - 1U + sizeof(Lcd1602DeviceConfigV1);
}

bool decodeLcd1602DeviceConfig(const uint8_t* blob, size_t size, Lcd1602DeviceConfigV1& config);

} // namespace ewfm
