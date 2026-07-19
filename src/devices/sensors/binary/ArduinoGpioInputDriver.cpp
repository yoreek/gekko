#include "devices/sensors/binary/ArduinoGpioInputDriver.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace ewfm {

bool ArduinoGpioInputDriver::configureInput(uint8_t pin, GpioInputPullMode pullMode) {
#if defined(ARDUINO)
    switch (pullMode) {
    case GpioInputPullMode::PullUp:
        pinMode(pin, INPUT_PULLUP);
        break;
    case GpioInputPullMode::PullDown:
        pinMode(pin, INPUT_PULLDOWN);
        break;
    case GpioInputPullMode::None:
    default:
        pinMode(pin, INPUT);
        break;
    }
    return true;
#else
    (void)pin;
    (void)pullMode;
    return true;
#endif
}

bool ArduinoGpioInputDriver::read(uint8_t pin, bool& level) {
#if defined(ARDUINO)
    level = digitalRead(pin) == HIGH;
    return true;
#else
    (void)pin;
    level = false;
    return true;
#endif
}

void ArduinoGpioInputDriver::release(uint8_t pin) {
#if defined(ARDUINO)
    pinMode(pin, INPUT);
#else
    (void)pin;
#endif
}

ArduinoGpioInputDriver& defaultArduinoGpioInputDriver() {
    static ArduinoGpioInputDriver driver;
    return driver;
}

} // namespace ewfm
