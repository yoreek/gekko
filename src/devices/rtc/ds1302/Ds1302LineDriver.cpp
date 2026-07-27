#include "devices/rtc/ds1302/Ds1302LineDriver.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace ewfm {

bool ArduinoDs1302LineDriver::setClk(uint8_t pin, bool high) {
#if defined(ARDUINO)
    pinMode(pin, OUTPUT);
    digitalWrite(pin, high ? HIGH : LOW);
    return true;
#else
    (void)pin;
    (void)high;
    return true;
#endif
}

bool ArduinoDs1302LineDriver::setRst(uint8_t pin, bool high) {
#if defined(ARDUINO)
    pinMode(pin, OUTPUT);
    digitalWrite(pin, high ? HIGH : LOW);
    return true;
#else
    (void)pin;
    (void)high;
    return true;
#endif
}

bool ArduinoDs1302LineDriver::setDataOutput(uint8_t pin) {
#if defined(ARDUINO)
    pinMode(pin, OUTPUT);
    return true;
#else
    (void)pin;
    return true;
#endif
}

bool ArduinoDs1302LineDriver::setDataInput(uint8_t pin) {
#if defined(ARDUINO)
    pinMode(pin, INPUT);
    return true;
#else
    (void)pin;
    return true;
#endif
}

bool ArduinoDs1302LineDriver::writeData(uint8_t pin, bool high) {
#if defined(ARDUINO)
    digitalWrite(pin, high ? HIGH : LOW);
    return true;
#else
    (void)pin;
    (void)high;
    return true;
#endif
}

bool ArduinoDs1302LineDriver::readData(uint8_t pin, bool& level) {
#if defined(ARDUINO)
    level = digitalRead(pin) == HIGH;
    return true;
#else
    (void)pin;
    level = false;
    return true;
#endif
}

void ArduinoDs1302LineDriver::waitMicros(uint32_t microseconds) {
#if defined(ARDUINO)
    delayMicroseconds(microseconds);
#else
    (void)microseconds;
#endif
}

ArduinoDs1302LineDriver& defaultArduinoDs1302LineDriver() {
    static ArduinoDs1302LineDriver driver;
    return driver;
}

} // namespace ewfm
