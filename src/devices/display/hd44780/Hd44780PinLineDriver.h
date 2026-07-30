#pragma once

#include <cstdint>

namespace ewfm {

// Testable hardware seam for direct-GPIO HD44780 wiring (mirrors ITm1637LineDriver's shape).
// Lines are addressed by a fixed logical index (0=RS, 1=E, 2-5=D4-D7, 6=Backlight-optional), not a
// dependency slot -- Hd44780PinCharacterDisplayDeviceBase maps each index to a raw pin number from
// its config and this driver just drives the requested level. Every method returns bool purely as a
// test seam; real GPIO writes never fail, so ArduinoHd44780PinLineDriver always returns true.
class IHd44780PinLineDriver {
public:
    IHd44780PinLineDriver() = default;
    IHd44780PinLineDriver(const IHd44780PinLineDriver&) = delete;
    IHd44780PinLineDriver& operator=(const IHd44780PinLineDriver&) = delete;
    virtual ~IHd44780PinLineDriver() = default;

    // `pins` holds `count` entries (6 or 7, index order as above); an entry of 0xFF (unset
    // backlight) is skipped.
    virtual bool configure(const uint8_t* pins, uint8_t count) = 0;
    virtual bool setLine(uint8_t lineIndex, bool level) = 0;
    virtual void release(const uint8_t* pins, uint8_t count) = 0;
};

class ArduinoHd44780PinLineDriver final : public IHd44780PinLineDriver {
public:
    bool configure(const uint8_t* pins, uint8_t count) override;
    bool setLine(uint8_t lineIndex, bool level) override;
    void release(const uint8_t* pins, uint8_t count) override;

private:
    static constexpr uint8_t kMaxLines = 7U;
    static constexpr uint8_t kUnsetPin = 0xFFU;

    uint8_t pins_[kMaxLines]{};
};

ArduinoHd44780PinLineDriver& defaultArduinoHd44780PinLineDriver();

} // namespace ewfm
