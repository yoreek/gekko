#pragma once

#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

// Sentinel for backlightChannel meaning "not wired" -- the backlight line is the one optional
// channel (some parallel-wired boards tie it permanently high instead of giving it a PCF857x pin).
constexpr uint8_t kHd44780ChannelUnset = 0xFFU;
constexpr uint8_t kHd44780MaxChannel = 15U; // PCF8575 tops out at 16 channels (0-15).

#pragma pack(push, 1)
// Shared by every HD44780-via-port-expander display type (lcd1602, lcd2004, ...): the 7 channel
// indices on the dependency's DeviceRole::PortExpander device (a pcf8574_expander/pcf8575_expander)
// that drive the HD44780 4-bit interface, plus backlight. Channels are port-expander channel
// indices, not raw controller GPIO pins. Defaults match the near-universal PCF8574 LCM-IIC backpack
// wiring (RS=P0, RW=P1 unused/tied low, E=P2, Backlight=P3, D4-D7=P4-P7). Composed as a field on
// each concrete config (not inherited from a base struct), matching how SensorFilterConfigV1 etc.
// are shared per docs/device-model-structures.md -- parseJson/writeJson operate on the same
// top-level JSON object as the embedding config, so the wire shape stays flat.
struct Hd44780ChannelConfigV1 {
    uint8_t rsChannel{0U};
    uint8_t eChannel{2U};
    uint8_t d4Channel{4U};
    uint8_t d5Channel{5U};
    uint8_t d6Channel{6U};
    uint8_t d7Channel{7U};
    uint8_t backlightChannel{3U}; // kHd44780ChannelUnset = not wired

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

// Writes up to maxOut reserved (non-kHd44780ChannelUnset) port-expander channels from `config` into
// `out` and returns how many were written. Shared by each display type's expanderChannels()
// (duplicate-channel protection at runtime) and its REST adapter's create/update validation (before
// a runtime exists yet).
uint8_t hd44780ConfigChannels(const Hd44780ChannelConfigV1& config, uint8_t* out, uint8_t maxOut);

} // namespace ewfm
