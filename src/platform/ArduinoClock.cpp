#include "platform/ArduinoClock.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#endif

namespace ewfm {

uint32_t ArduinoClock::millis() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return ::millis();
#else
    return 0;
#endif
}

} // namespace ewfm
