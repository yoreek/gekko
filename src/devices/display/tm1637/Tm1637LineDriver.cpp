#include "devices/display/tm1637/Tm1637LineDriver.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace ewfm {

bool ArduinoTm1637LineDriver::configure(uint8_t clkPin, uint8_t dioPin) {
#if defined(ARDUINO)
    digitalWrite(clkPin, HIGH);
    pinMode(clkPin, OUTPUT);
    digitalWrite(dioPin, HIGH);
    pinMode(dioPin, OUTPUT);
    return true;
#else
    (void)clkPin;
    (void)dioPin;
    return true;
#endif
}

bool ArduinoTm1637LineDriver::setClock(uint8_t clkPin, bool high) {
#if defined(ARDUINO)
    digitalWrite(clkPin, high ? HIGH : LOW);
    return true;
#else
    (void)clkPin;
    (void)high;
    return true;
#endif
}

bool ArduinoTm1637LineDriver::driveData(uint8_t dioPin, bool high) {
#if defined(ARDUINO)
    digitalWrite(dioPin, high ? HIGH : LOW);
    pinMode(dioPin, OUTPUT);
    return true;
#else
    (void)dioPin;
    (void)high;
    return true;
#endif
}

bool ArduinoTm1637LineDriver::releaseData(uint8_t dioPin) {
#if defined(ARDUINO)
    // Pull-up so the line idles high while the chip is free to pull it low for the ACK bit. Most
    // TM1637 breakout boards also carry their own pull-ups; the internal one just covers bare chips.
    pinMode(dioPin, INPUT_PULLUP);
    return true;
#else
    (void)dioPin;
    return true;
#endif
}

bool ArduinoTm1637LineDriver::readData(uint8_t dioPin, bool& level) {
#if defined(ARDUINO)
    level = digitalRead(dioPin) == HIGH;
    return true;
#else
    (void)dioPin;
    level = false;
    return true;
#endif
}

void ArduinoTm1637LineDriver::waitMicros(uint32_t microseconds) {
#if defined(ARDUINO)
    delayMicroseconds(microseconds);
#else
    (void)microseconds;
#endif
}

void ArduinoTm1637LineDriver::release(uint8_t clkPin, uint8_t dioPin) {
#if defined(ARDUINO)
    pinMode(clkPin, INPUT);
    pinMode(dioPin, INPUT);
#else
    (void)clkPin;
    (void)dioPin;
#endif
}

ArduinoTm1637LineDriver& defaultArduinoTm1637LineDriver() {
    static ArduinoTm1637LineDriver driver;
    return driver;
}

} // namespace ewfm
