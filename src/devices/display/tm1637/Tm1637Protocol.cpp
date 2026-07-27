#include "devices/display/tm1637/Tm1637Protocol.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#endif

namespace ewfm {
namespace {
void settleMicros(uint32_t value) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    delayMicroseconds(value);
#else
    (void)value;
#endif
}
} // namespace

Tm1637Protocol::Tm1637Protocol(ITm1637LineDriver& lines) : lines_(lines) {}

void Tm1637Protocol::settle() {
    settleMicros(2U);
}

bool Tm1637Protocol::start(const uint32_t now) {
    return lines_.setData(true, now) && lines_.setClock(true, now) && (settle(), true) && lines_.setData(false, now);
}

bool Tm1637Protocol::stop(const uint32_t now) {
    return lines_.setClock(false, now) && (settle(), true) && lines_.setData(false, now) && (settle(), true) &&
           lines_.setClock(true, now) && (settle(), true) && lines_.setData(true, now);
}

bool Tm1637Protocol::writeByte(const uint8_t value, const uint32_t now) {
    bool ok = true;
    for (uint8_t bit = 0U; bit < 8U; ++bit) {
        ok = lines_.setClock(false, now) && ok;
        ok = lines_.setData(((value >> bit) & 0x01U) != 0U, now) && ok;
        settle();
        ok = lines_.setClock(true, now) && ok;
        settle();
    }
    ok = lines_.setClock(false, now) && ok;
    return ok;
}

bool Tm1637Protocol::writeFrame(const uint8_t* digits, const uint8_t digitCount, const uint8_t brightness, const uint32_t now) {
    if (digits == nullptr || digitCount == 0U) {
        return false;
    }

    bool ok = start(now);
    ok = writeByte(0x40U, now) && ok;
    ok = stop(now) && ok;
    ok = start(now) && ok;
    ok = writeByte(0xC0U, now) && ok;
    for (uint8_t index = 0U; index < digitCount; ++index) {
        ok = writeByte(digits[index], now) && ok;
    }
    ok = stop(now) && ok;
    ok = start(now) && ok;
    ok = writeByte(static_cast<uint8_t>(0x88U | (brightness & 0x07U)), now) && ok;
    ok = stop(now) && ok;
    return ok;
}

bool Tm1637Protocol::displayOff(const uint32_t now) {
    bool ok = start(now);
    ok = writeByte(0x80U, now) && ok;
    ok = stop(now) && ok;
    return ok;
}

} // namespace ewfm
