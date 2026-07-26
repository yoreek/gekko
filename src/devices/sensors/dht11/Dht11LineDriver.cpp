#include "devices/sensors/dht11/Dht11LineDriver.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace ewfm {

bool ArduinoDht11LineDriver::driveLow(uint8_t pin) {
#if defined(ARDUINO)
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    return true;
#else
    (void)pin;
    return true;
#endif
}

bool ArduinoDht11LineDriver::release(uint8_t pin) {
#if defined(ARDUINO)
    pinMode(pin, INPUT);
    return true;
#else
    (void)pin;
    return true;
#endif
}

bool ArduinoDht11LineDriver::read(uint8_t pin, bool& level) {
#if defined(ARDUINO)
    level = digitalRead(pin) == HIGH;
    return true;
#else
    (void)pin;
    level = true;
    return true;
#endif
}

void ArduinoDht11LineDriver::waitMicros(uint32_t microseconds) {
#if defined(ARDUINO)
    delayMicroseconds(microseconds);
#else
    (void)microseconds;
#endif
}

ArduinoDht11LineDriver& defaultArduinoDht11LineDriver() {
    static ArduinoDht11LineDriver driver;
    return driver;
}

} // namespace ewfm
