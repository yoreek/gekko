#pragma once

#include <cstdint>

namespace ewfm {

// Testable hardware seam for TM1637's 2-wire bit-bang interface, mirroring IDs1302LineDriver's
// shape. CLK is a plain output; DIO is bidirectional because the chip pulls it low to acknowledge
// each byte, so it needs an explicit direction switch. Every method returns bool purely as a test
// seam (a fake driver can simulate a stuck/absent chip by returning false) - real GPIO writes/reads
// never fail this way, so ArduinoTm1637LineDriver always returns true.
class ITm1637LineDriver {
public:
    ITm1637LineDriver() = default;
    ITm1637LineDriver(const ITm1637LineDriver&) = delete;
    ITm1637LineDriver& operator=(const ITm1637LineDriver&) = delete;
    virtual ~ITm1637LineDriver() = default;

    virtual bool configure(uint8_t clkPin, uint8_t dioPin) = 0;
    virtual bool setClock(uint8_t clkPin, bool high) = 0;
    virtual bool driveData(uint8_t dioPin, bool high) = 0;
    virtual bool releaseData(uint8_t dioPin) = 0;
    virtual bool readData(uint8_t dioPin, bool& level) = 0;
    virtual void waitMicros(uint32_t microseconds) = 0;
    virtual void release(uint8_t clkPin, uint8_t dioPin) = 0;
};

class ArduinoTm1637LineDriver final : public ITm1637LineDriver {
public:
    bool configure(uint8_t clkPin, uint8_t dioPin) override;
    bool setClock(uint8_t clkPin, bool high) override;
    bool driveData(uint8_t dioPin, bool high) override;
    bool releaseData(uint8_t dioPin) override;
    bool readData(uint8_t dioPin, bool& level) override;
    void waitMicros(uint32_t microseconds) override;
    void release(uint8_t clkPin, uint8_t dioPin) override;
};

ArduinoTm1637LineDriver& defaultArduinoTm1637LineDriver();

} // namespace ewfm
