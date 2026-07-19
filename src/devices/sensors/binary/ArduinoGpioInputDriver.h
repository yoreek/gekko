#pragma once

#include "devices/sensors/binary/IGpioInputDriver.h"

namespace ewfm {

class ArduinoGpioInputDriver final : public IGpioInputDriver {
public:
    bool configureInput(uint8_t pin, GpioInputPullMode pullMode) override;
    bool read(uint8_t pin, bool& level) override;
    void release(uint8_t pin) override;
};

ArduinoGpioInputDriver& defaultArduinoGpioInputDriver();

} // namespace ewfm
