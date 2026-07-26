#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/hd44780/Hd44780ChannelConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr size_t kLcd2004LineLength = 20U; // HD44780 2004 panel: 20 columns.

#pragma pack(push, 1)
struct Lcd2004DeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "LCD2004-1";

    Hd44780ChannelConfigV1 channels{};
    char line1[kLcd2004LineLength + 1U]{};
    char line2[kLcd2004LineLength + 1U]{};
    char line3[kLcd2004LineLength + 1U]{};
    char line4[kLcd2004LineLength + 1U]{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t lcd2004DeviceConfigSize(const Lcd2004DeviceConfigV1&) {
    return sizeof(Lcd2004DeviceConfigV1::kMagic) - 1U + sizeof(Lcd2004DeviceConfigV1);
}

bool decodeLcd2004DeviceConfig(const uint8_t* blob, size_t size, Lcd2004DeviceConfigV1& config);

} // namespace ewfm
