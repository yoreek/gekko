#pragma once

#include <cstdint>

namespace ewfm {

class IGpioOutputDriver {
public:
    IGpioOutputDriver() = default;
    IGpioOutputDriver(const IGpioOutputDriver&) = delete;
    IGpioOutputDriver& operator=(const IGpioOutputDriver&) = delete;
    virtual ~IGpioOutputDriver() = default;

    virtual bool configureOutput(uint8_t pin, bool initialLevel) = 0;
    virtual bool write(uint8_t pin, bool level) = 0;
    virtual void release(uint8_t pin) = 0;
};

} // namespace ewfm
