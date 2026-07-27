#pragma once

#include <cstdint>

namespace ewfm {

// Testable hardware seam for DS1302's 3-wire bit-bang interface, mirroring IDht11LineDriver's
// shape. DAT is bidirectional so it needs an explicit direction switch; CLK/RST are plain outputs.
// Every method returns bool purely as a test seam (a fake driver can simulate a stuck/absent chip
// by returning false) - real GPIO writes/reads never fail this way, so ArduinoDs1302LineDriver
// always returns true.
class IDs1302LineDriver {
public:
    IDs1302LineDriver() = default;
    IDs1302LineDriver(const IDs1302LineDriver&) = delete;
    IDs1302LineDriver& operator=(const IDs1302LineDriver&) = delete;
    virtual ~IDs1302LineDriver() = default;

    virtual bool setClk(uint8_t pin, bool high) = 0;
    virtual bool setRst(uint8_t pin, bool high) = 0;
    virtual bool setDataOutput(uint8_t pin) = 0;
    virtual bool setDataInput(uint8_t pin) = 0;
    virtual bool writeData(uint8_t pin, bool high) = 0;
    virtual bool readData(uint8_t pin, bool& level) = 0;
    virtual void waitMicros(uint32_t microseconds) = 0;
};

class ArduinoDs1302LineDriver final : public IDs1302LineDriver {
public:
    bool setClk(uint8_t pin, bool high) override;
    bool setRst(uint8_t pin, bool high) override;
    bool setDataOutput(uint8_t pin) override;
    bool setDataInput(uint8_t pin) override;
    bool writeData(uint8_t pin, bool high) override;
    bool readData(uint8_t pin, bool& level) override;
    void waitMicros(uint32_t microseconds) override;
};

ArduinoDs1302LineDriver& defaultArduinoDs1302LineDriver();

} // namespace ewfm
