#pragma once

namespace ewfm {

struct DeviceCommand;

inline constexpr bool kSwitchOutputOff = false;
inline constexpr bool kSwitchOutputOn = true;

bool parseSwitchOutputStateCommand(const DeviceCommand& command, bool& state);
const char* switchOutputStateName(bool state);

} // namespace ewfm
