#include "devices/display/tm1637/Tm1637LineDriver.h"

namespace ewfm {

SwitchTm1637LineDriver::SwitchTm1637LineDriver(ISwitchOutputRuntime& clockLine, ISwitchOutputRuntime& dataLine)
    : clockLine_(clockLine), dataLine_(dataLine) {}

bool SwitchTm1637LineDriver::setClock(const bool level, const uint32_t now) {
    return clockLine_.requestOutputState(level, now);
}

bool SwitchTm1637LineDriver::setData(const bool level, const uint32_t now) {
    return dataLine_.requestOutputState(level, now);
}

} // namespace ewfm
