#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

// Sentinel for backlightChannel meaning "not wired" -- the backlight line is the one optional
// channel (some parallel-wired boards tie it permanently high instead of giving it a PCF857x pin).
constexpr uint8_t kLcd1602ChannelUnset = 0xFFU;
constexpr uint8_t kLcd1602MaxChannel = 15U; // PCF8575 tops out at 16 channels (0-15).
constexpr size_t kLcd1602LineLength = 16U;  // HD44780 1602 panel: 16 columns.

#pragma pack(push, 1)
// Channels are indices on the dependency's DeviceRole::PortExpander device (a pcf8574_expander/
// pcf8575_expander), not raw controller GPIO pins -- see docs note in Lcd1602Device.h. Defaults
// match the near-universal PCF8574 LCM1602-IIC backpack wiring (RS=P0, RW=P1 unused/tied low,
// E=P2, Backlight=P3, D4-D7=P4-P7).
struct Lcd1602DeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "LCD1602-1";

    uint8_t rsChannel{0U};
    uint8_t eChannel{2U};
    uint8_t d4Channel{4U};
    uint8_t d5Channel{5U};
    uint8_t d6Channel{6U};
    uint8_t d7Channel{7U};
    uint8_t backlightChannel{3U}; // kLcd1602ChannelUnset = not wired
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

// Writes up to maxOut reserved (non-kLcd1602ChannelUnset) port-expander channels from `config` into
// `out` and returns how many were written. Shared by Lcd1602Device::expanderChannels() (duplicate-
// channel protection at runtime) and the REST adapter's create/update validation (before a runtime
// exists yet).
uint8_t lcd1602ConfigChannels(const Lcd1602DeviceConfigV1& config, uint8_t* out, uint8_t maxOut);

} // namespace ewfm
