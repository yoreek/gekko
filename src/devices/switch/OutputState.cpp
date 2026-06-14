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

bool outputStateFromByte(uint8_t value, OutputState& state) {
    const OutputState candidate = static_cast<OutputState>(value);
    if (!outputStateIsValid(candidate)) {
        return false;
    }
    state = candidate;
    return true;
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
