#include "devices/switch/gpio/ArduinoGpioOutputDriver.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace ewfm {

bool ArduinoGpioOutputDriver::configureOutput(uint8_t pin, bool initialLevel) {
#if defined(ARDUINO)
    digitalWrite(pin, initialLevel ? HIGH : LOW);
    pinMode(pin, OUTPUT);
    return true;
#else
    (void)pin;
    (void)initialLevel;
    return true;
#endif
}

bool ArduinoGpioOutputDriver::write(uint8_t pin, bool level) {
#if defined(ARDUINO)
    digitalWrite(pin, level ? HIGH : LOW);
    return true;
#else
    (void)pin;
    (void)level;
    return true;
#endif
}

bool ArduinoGpioOutputDriver::disableOutput(uint8_t pin) {
#if defined(ARDUINO)
    pinMode(pin, INPUT);
    return true;
#else
    (void)pin;
    return true;
#endif
}

void ArduinoGpioOutputDriver::release(uint8_t pin) {
#if defined(ARDUINO)
    pinMode(pin, INPUT);
#else
    (void)pin;
#endif
}

ArduinoGpioOutputDriver& defaultArduinoGpioOutputDriver() {
    static ArduinoGpioOutputDriver driver;
    return driver;
}

} // namespace ewfm
