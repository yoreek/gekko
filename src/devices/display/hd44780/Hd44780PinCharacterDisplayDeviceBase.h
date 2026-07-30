#pragma once

#include "devices/display/hd44780/Hd44780CharacterDisplayDeviceBase.h"
#include "devices/display/hd44780/Hd44780PinLineDriver.h"

namespace ewfm {

// Read-only snapshot of the raw ESP32 GPIO pins a concrete leaf's direct-pin config carries for
// wiring, in the same line order as Hd44780I2cLineChannels but with pin numbers instead of PCF8574
// bit positions.
struct Hd44780PinLineChannels {
    uint8_t rsPin;
    uint8_t ePin;
    uint8_t d4Pin;
    uint8_t d5Pin;
    uint8_t d6Pin;
    uint8_t d7Pin;
    uint8_t backlightPin; // kHd44780PinUnset = not wired
};

// Direct-GPIO transport for HD44780 displays wired straight to ESP32 pins - no I2C, no dependency,
// same ownership model as Tm1637Device/Ds1302RtcDevice. Talks to the pins through
// IHd44780PinLineDriver so the protocol logic in Hd44780CharacterDisplayDeviceBase stays testable
// without real hardware.
class Hd44780PinCharacterDisplayDeviceBase : public Hd44780CharacterDisplayDeviceBase {
protected:
    Hd44780PinCharacterDisplayDeviceBase(PState initialState, uint8_t columns, uint8_t rows);
    Hd44780PinCharacterDisplayDeviceBase(PState initialState, uint8_t columns, uint8_t rows, IHd44780PinLineDriver& lineDriver);

    // Supplied by the leaf's config (see Hd44780PinLeafDeviceBase).
    virtual Hd44780PinLineChannels pinLineChannels() const = 0;

    bool initializeDisplayHardware(uint32_t now) override;
    void releaseDisplayHardware(uint32_t now) override;
    bool setLine(uint8_t lineIndex, bool level, uint32_t now) final;

private:
    IHd44780PinLineDriver& lines_;
};

} // namespace ewfm
