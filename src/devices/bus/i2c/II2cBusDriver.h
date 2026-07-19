#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

class TwoWire;

namespace ewfm {

class II2cBusDriver {
public:
    II2cBusDriver() = default;
    II2cBusDriver(const II2cBusDriver&) = delete;
    II2cBusDriver& operator=(const II2cBusDriver&) = delete;
    II2cBusDriver(II2cBusDriver&&) = delete;
    II2cBusDriver& operator=(II2cBusDriver&&) = delete;
    virtual ~II2cBusDriver() = default;

    virtual bool begin(uint8_t sdaPin, uint8_t sclPin, uint32_t frequencyHz, bool internalPullup) = 0;
    virtual bool end() = 0;
    virtual bool setClock(uint32_t frequencyHz) = 0;
    virtual uint32_t getClock() const = 0;
    virtual void beginTransmission(uint8_t address) = 0;
    virtual uint8_t endTransmission(bool sendStop = true) = 0;
    virtual size_t requestFrom(uint8_t address, size_t size, bool sendStop = true) = 0;
    virtual size_t write(uint8_t data) = 0;
    virtual size_t write(const uint8_t* data, size_t quantity) = 0;
    virtual int available() = 0;
    virtual int read() = 0;
    virtual void flush() = 0;
    virtual TwoWire* nativeWire() const {
        return nullptr;
    }
};

II2cBusDriver& defaultArduinoI2cBusDriver();
std::unique_ptr<II2cBusDriver> createArduinoI2cBusDriver();

} // namespace ewfm
