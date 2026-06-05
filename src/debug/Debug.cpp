#include "debug/Debug.h"

#include <cstdarg>
#include <cstdio>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#endif

namespace ewfm {

namespace {
DebugLogger globalLogger;
}

DebugLogger& debugLogger() {
    return globalLogger;
}

void DebugLogger::begin() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
#ifndef DEBUG_SERIAL_DEVICE
#define DEBUG_SERIAL_DEVICE Serial
#endif
#ifndef DEBUG_SERIAL_SPEED
#define DEBUG_SERIAL_SPEED 115200
#endif
    DEBUG_SERIAL_DEVICE.begin(DEBUG_SERIAL_SPEED);
#endif
}

bool DebugLogger::enabled(DebugLevel level) const {
    return static_cast<int>(level) <= static_cast<int>(level_);
}

void DebugLogger::log(DebugLevel level, const char* domain, const char* file, int line, const char* function, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(level, domain, file, line, function, format, args);
    va_end(args);
}

void DebugLogger::vlog(DebugLevel level, const char* domain, const char* file, int line, const char* function, const char* format,
                       va_list args) {
    if (!enabled(level)) {
        return;
    }

#if defined(ARDUINO) && !defined(UNIT_TEST)
    char message[192];
    std::vsnprintf(message, sizeof(message), format, args);
    DEBUG_SERIAL_DEVICE.printf("[%c] %s %s:%d %s: ", debugLevelChar(level), domain, file, line, function);
    DEBUG_SERIAL_DEVICE.println(message);
#else
    std::fprintf(stderr, "[%c] %s %s:%d %s: ", debugLevelChar(level), domain, file, line, function);
    std::vfprintf(stderr, format, args);
    std::fprintf(stderr, "\n");
#endif
}

} // namespace ewfm
