#include "devices/display/tm1637/Tm1637Protocol.h"

namespace ewfm {

Tm1637Protocol::Tm1637Protocol(ITm1637LineDriver& lines, const uint8_t clkPin, const uint8_t dioPin)
    : lines_(lines), clkPin_(clkPin), dioPin_(dioPin) {}

void Tm1637Protocol::settle() {
    lines_.waitMicros(kTm1637BitDelayMicros);
}

bool Tm1637Protocol::start() {
    bool ok = lines_.setClock(clkPin_, true);
    ok = lines_.driveData(dioPin_, true) && ok;
    settle();
    ok = lines_.driveData(dioPin_, false) && ok;
    settle();
    return ok;
}

bool Tm1637Protocol::stop() {
    bool ok = lines_.setClock(clkPin_, false);
    ok = lines_.driveData(dioPin_, false) && ok;
    settle();
    ok = lines_.setClock(clkPin_, true) && ok;
    settle();
    ok = lines_.driveData(dioPin_, true) && ok;
    settle();
    return ok;
}

bool Tm1637Protocol::writeByte(const uint8_t value) {
    bool ok = true;
    for (uint8_t bit = 0U; bit < 8U; ++bit) {
        ok = lines_.setClock(clkPin_, false) && ok;
        ok = lines_.driveData(dioPin_, ((value >> bit) & 0x01U) != 0U) && ok;
        settle();
        ok = lines_.setClock(clkPin_, true) && ok;
        settle();
    }

    // Ninth clock: the chip acknowledges by holding DIO low while CLK is high. Skipping this pulse
    // leaves the chip waiting mid-byte and every following byte is discarded.
    ok = lines_.setClock(clkPin_, false) && ok;
    ok = lines_.releaseData(dioPin_) && ok;
    settle();
    ok = lines_.setClock(clkPin_, true) && ok;
    settle();
    bool acknowledged = false;
    bool level = true;
    if (lines_.readData(dioPin_, level)) {
        acknowledged = !level;
    } else {
        ok = false;
    }
    ok = lines_.setClock(clkPin_, false) && ok;
    // The chip lets go of DIO on the falling edge, so taking the line back is safe only now.
    ok = lines_.driveData(dioPin_, false) && ok;
    settle();
    return ok && acknowledged;
}

bool Tm1637Protocol::writeFrame(const uint8_t* digits, const uint8_t digitCount, const uint8_t brightness) {
    if (digits == nullptr || digitCount == 0U) {
        return false;
    }

    // Data command: write to display register, auto-incrementing address.
    bool ok = start();
    ok = writeByte(0x40U) && ok;
    ok = stop() && ok;

    // Address command 0xC0 followed by the digit bytes.
    ok = start() && ok;
    ok = writeByte(0xC0U) && ok;
    for (uint8_t index = 0U; index < digitCount; ++index) {
        ok = writeByte(digits[index]) && ok;
    }
    ok = stop() && ok;

    // Display control: on, with the configured brightness.
    ok = start() && ok;
    ok = writeByte(static_cast<uint8_t>(0x88U | (brightness & 0x07U))) && ok;
    ok = stop() && ok;
    return ok;
}

bool Tm1637Protocol::displayOff() {
    bool ok = start();
    ok = writeByte(0x80U) && ok;
    ok = stop() && ok;
    return ok;
}

} // namespace ewfm
