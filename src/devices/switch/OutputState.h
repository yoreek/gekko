#pragma once

#include <cstdint>

namespace ewfm {

enum class OutputState : uint8_t {
    Off = 0,
    On = 1,
    Disabled = 2,
};

using OutputStateMask = uint8_t;

constexpr OutputStateMask kOutputStateMaskOff = 1U << static_cast<uint8_t>(OutputState::Off);
constexpr OutputStateMask kOutputStateMaskOn = 1U << static_cast<uint8_t>(OutputState::On);
constexpr OutputStateMask kOutputStateMaskDisabled = 1U << static_cast<uint8_t>(OutputState::Disabled);
constexpr OutputStateMask kOutputStateMaskBinary = kOutputStateMaskOff | kOutputStateMaskOn;
constexpr OutputStateMask kOutputStateMaskTriState = kOutputStateMaskBinary | kOutputStateMaskDisabled;

bool outputStateIsValid(OutputState state);
bool outputStateIsSupported(OutputState state, OutputStateMask mask);
bool outputStateFromByte(uint8_t value, OutputState& state);
const char* outputStateName(OutputState state);

} // namespace ewfm
