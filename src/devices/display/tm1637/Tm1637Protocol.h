#pragma once

#include "devices/display/tm1637/Tm1637LineDriver.h"

#include <cstdint>

namespace ewfm {

class Tm1637Protocol final {
public:
    explicit Tm1637Protocol(ITm1637LineDriver& lines);

    bool writeFrame(const uint8_t* digits, uint8_t digitCount, uint8_t brightness, uint32_t now);
    bool displayOff(uint32_t now);

private:
    bool start(uint32_t now);
    bool stop(uint32_t now);
    bool writeByte(uint8_t value, uint32_t now);
    void settle();

    ITm1637LineDriver& lines_;
};

} // namespace ewfm
