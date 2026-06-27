#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace ewfm {

class ISpiBusDriver {
public:
    ISpiBusDriver() = default;
    ISpiBusDriver(const ISpiBusDriver&) = delete;
    ISpiBusDriver& operator=(const ISpiBusDriver&) = delete;
    ISpiBusDriver(ISpiBusDriver&&) = delete;
    ISpiBusDriver& operator=(ISpiBusDriver&&) = delete;
    virtual ~ISpiBusDriver() = default;

    virtual bool begin(uint8_t host, uint8_t sckPin, uint8_t mosiPin, int16_t misoPin) = 0;
    virtual bool end() = 0;
    virtual bool beginTransaction(uint32_t clockHz, uint8_t dataMode, uint8_t bitOrder) = 0;
    virtual void endTransaction() = 0;
    virtual uint8_t transfer(uint8_t data) = 0;
    virtual size_t transferBytes(const uint8_t* txData, uint8_t* rxData, size_t length) = 0;
};

ISpiBusDriver& defaultArduinoSpiBusDriver();
std::unique_ptr<ISpiBusDriver> createArduinoSpiBusDriver();

} // namespace ewfm
