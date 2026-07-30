#pragma once

#include "devices/display/tm1637/Tm1637LineDriver.h"

#include <cstdint>

namespace ewfm {

// Half-cycle hold time. The datasheet only bounds the clock from above (250 kHz), but the RC
// filters on typical TM1637 breakout boards need far slower edges than a bare chip, so this matches
// the 100us that field-proven TM1637 libraries settle on. A full frame is ~7 bytes, i.e. ~13ms of
// busy-wait, and the display only re-transmits when the rendered digits actually change.
constexpr uint32_t kTm1637BitDelayMicros = 100U;

// Bit-bang transport for TM1637. The framing is I2C-like (START/STOP, one ACK clock per byte) but
// the payload is LSB-first and there is no device address, so it cannot ride on hardware I2C.
class Tm1637Protocol final {
public:
    Tm1637Protocol(ITm1637LineDriver& lines, uint8_t clkPin, uint8_t dioPin);

    bool writeFrame(const uint8_t* digits, uint8_t digitCount, uint8_t brightness);
    bool displayOff();

private:
    bool start();
    bool stop();
    bool writeByte(uint8_t value);
    void settle();

    ITm1637LineDriver& lines_;
    uint8_t clkPin_;
    uint8_t dioPin_;
};

} // namespace ewfm
