#include "devices/display/hd44780/Hd44780PinCharacterDisplayDeviceBase.h"

namespace ewfm {

Hd44780PinCharacterDisplayDeviceBase::Hd44780PinCharacterDisplayDeviceBase(PState initialState, uint8_t columns, uint8_t rows)
    : Hd44780PinCharacterDisplayDeviceBase(initialState, columns, rows, defaultArduinoHd44780PinLineDriver()) {}

Hd44780PinCharacterDisplayDeviceBase::Hd44780PinCharacterDisplayDeviceBase(PState initialState, const uint8_t columns, const uint8_t rows,
                                                                           IHd44780PinLineDriver& lineDriver)
    : Hd44780CharacterDisplayDeviceBase(initialState, columns, rows), lines_(lineDriver) {}

bool Hd44780PinCharacterDisplayDeviceBase::initializeDisplayHardware(const uint32_t now) {
    const Hd44780PinLineChannels channels = pinLineChannels();
    const uint8_t pins[7] = {
        channels.rsPin, channels.ePin, channels.d4Pin, channels.d5Pin, channels.d6Pin, channels.d7Pin, channels.backlightPin,
    };
    if (!lines_.configure(pins, 7U)) {
        return false;
    }
    return Hd44780CharacterDisplayDeviceBase::initializeDisplayHardware(now);
}

void Hd44780PinCharacterDisplayDeviceBase::releaseDisplayHardware(const uint32_t now) {
    Hd44780CharacterDisplayDeviceBase::releaseDisplayHardware(now);
    const Hd44780PinLineChannels channels = pinLineChannels();
    const uint8_t pins[7] = {
        channels.rsPin, channels.ePin, channels.d4Pin, channels.d5Pin, channels.d6Pin, channels.d7Pin, channels.backlightPin,
    };
    lines_.release(pins, 7U);
}

bool Hd44780PinCharacterDisplayDeviceBase::setLine(const uint8_t lineIndex, const bool level, const uint32_t now) {
    (void)now;
    if (lineIndex > kHd44780LineBacklight) {
        return false;
    }
    return lines_.setLine(lineIndex, level);
}

} // namespace ewfm
