#include "devices/display/hd44780/Hd44780PinLineDriver.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#include <cstring>

namespace ewfm {

bool ArduinoHd44780PinLineDriver::configure(const uint8_t* pins, const uint8_t count) {
    std::memset(pins_, kUnsetPin, sizeof(pins_));
    const uint8_t copyCount = count > kMaxLines ? kMaxLines : count;
    std::memcpy(pins_, pins, copyCount);
#if defined(ARDUINO)
    for (uint8_t index = 0U; index < copyCount; ++index) {
        if (pins_[index] == kUnsetPin) {
            continue;
        }
        digitalWrite(pins_[index], LOW);
        pinMode(pins_[index], OUTPUT);
    }
#endif
    return true;
}

bool ArduinoHd44780PinLineDriver::setLine(const uint8_t lineIndex, const bool level) {
    if (lineIndex >= kMaxLines || pins_[lineIndex] == kUnsetPin) {
        return true;
    }
#if defined(ARDUINO)
    digitalWrite(pins_[lineIndex], level ? HIGH : LOW);
#else
    (void)level;
#endif
    return true;
}

void ArduinoHd44780PinLineDriver::release(const uint8_t* pins, const uint8_t count) {
#if defined(ARDUINO)
    const uint8_t releaseCount = count > kMaxLines ? kMaxLines : count;
    for (uint8_t index = 0U; index < releaseCount; ++index) {
        if (pins[index] == kUnsetPin) {
            continue;
        }
        pinMode(pins[index], INPUT);
    }
#else
    (void)pins;
    (void)count;
#endif
    std::memset(pins_, kUnsetPin, sizeof(pins_));
}

ArduinoHd44780PinLineDriver& defaultArduinoHd44780PinLineDriver() {
    static ArduinoHd44780PinLineDriver driver;
    return driver;
}

} // namespace ewfm
