#pragma once

#include "devices/switch/gpio/IGpioOutputDriver.h"

namespace ewfm {

class ArduinoGpioOutputDriver final : public IGpioOutputDriver {
public:
    bool configureOutput(uint8_t pin, bool initialLevel) override;
    bool write(uint8_t pin, bool level) override;
    bool disableOutput(uint8_t pin) override;
    void release(uint8_t pin) override;
};

ArduinoGpioOutputDriver& defaultArduinoGpioOutputDriver();

} // namespace ewfm
