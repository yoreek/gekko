#pragma once

#include "devices/rtc/ds1302/Ds1302LineDriver.h"

#include <cstdint>

namespace ewfm {

// GPIO pin triple for DS1302's proprietary 3-wire interface. CLK/RST are plain outputs; DAT is
// bidirectional and switched between output (write) and input (read) per transaction.
struct Ds1302Pins {
    uint8_t clk;
    uint8_t data;
    uint8_t rst;
};

bool ds1302ReadTime(IDs1302LineDriver& driver, const Ds1302Pins& pins, uint32_t& outUtcEpoch);
bool ds1302WriteTime(IDs1302LineDriver& driver, const Ds1302Pins& pins, uint32_t utcEpoch);

} // namespace ewfm
