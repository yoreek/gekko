#pragma once

namespace ewfm {

enum class DebugLevel {
    Error = 1,
    Warn = 2,
    Info = 3,
    Debug = 4,
    Trace = 5,
};

const char* debugLevelName(DebugLevel level);
char debugLevelChar(DebugLevel level);

} // namespace ewfm
