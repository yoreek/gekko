#include "debug/DebugLevel.h"

namespace ewfm {

const char* debugLevelName(DebugLevel level) {
    switch (level) {
    case DebugLevel::Error:
        return "error";
    case DebugLevel::Warn:
        return "warn";
    case DebugLevel::Info:
        return "info";
    case DebugLevel::Debug:
        return "debug";
    case DebugLevel::Trace:
        return "trace";
    }
    return "unknown";
}

char debugLevelChar(DebugLevel level) {
    switch (level) {
    case DebugLevel::Error:
        return 'E';
    case DebugLevel::Warn:
        return 'W';
    case DebugLevel::Info:
        return 'I';
    case DebugLevel::Debug:
        return 'D';
    case DebugLevel::Trace:
        return 'T';
    }
    return '?';
}

} // namespace ewfm
