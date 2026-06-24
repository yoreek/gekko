#include "devices/switch/OutputState.h"

namespace ewfm {

bool outputStateIsValid(OutputState state) {
    return state == OutputState::Off || state == OutputState::On || state == OutputState::Disabled;
}

bool outputStateIsSupported(OutputState state, OutputStateMask mask) {
    if (!outputStateIsValid(state)) {
        return false;
    }
    return (mask & (1U << static_cast<uint8_t>(state))) != 0U;
}

const char* outputStateName(OutputState state) {
    switch (state) {
    case OutputState::Off:
        return "off";
    case OutputState::On:
        return "on";
    case OutputState::Disabled:
        return "disabled";
    }
    return "unknown";
}

} // namespace ewfm
