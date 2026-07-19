#include "metrics/MetricValueFormat.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace ewfm {

namespace {

const char* const kWeekdayLongNames[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char* const kWeekdayShortNames[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

bool appendChar(char* output, const size_t capacity, size_t& length, const char ch) {
    if (length + 1U >= capacity) {
        return false;
    }
    output[length] = ch;
    ++length;
    output[length] = '\0';
    return true;
}

bool appendString(char* output, const size_t capacity, size_t& length, const char* text) {
    for (const char* cursor = text; *cursor != '\0'; ++cursor) {
        if (!appendChar(output, capacity, length, *cursor)) {
            return false;
        }
    }
    return true;
}

bool appendNumber(char* output, const size_t capacity, size_t& length, const unsigned long value) {
    char buffer[12];
    std::snprintf(buffer, sizeof(buffer), "%lu", value);
    return appendString(output, capacity, length, buffer);
}

bool appendPadded(char* output, const size_t capacity, size_t& length, const unsigned long value, const int width) {
    char buffer[12];
    std::snprintf(buffer, sizeof(buffer), "%0*lu", width, value);
    return appendString(output, capacity, length, buffer);
}

// Copies the [literal] span (excluding the brackets) starting at `cursor` (which must point at
// '['). Advances `cursor` past the closing ']', or to the end of the string if unterminated.
bool consumeLiteralSpan(const char*& cursor, char* output, const size_t capacity, size_t& length) {
    const char* closing = std::strchr(cursor + 1, ']');
    if (closing == nullptr) {
        const bool ok = appendString(output, capacity, length, cursor + 1);
        cursor += std::strlen(cursor);
        return ok;
    }
    for (const char* literalCursor = cursor + 1; literalCursor < closing; ++literalCursor) {
        if (!appendChar(output, capacity, length, *literalCursor)) {
            return false;
        }
    }
    cursor = closing + 1;
    return true;
}

} // namespace

void formatDurationDefault(const uint32_t durationMs, char* output, const size_t capacity) {
    if (output == nullptr || capacity == 0U) {
        return;
    }
    const uint32_t totalSeconds = durationMs / 1000U;
    const uint32_t hours = totalSeconds / 3600U;
    const uint32_t minutes = (totalSeconds / 60U) % 60U;
    const uint32_t seconds = totalSeconds % 60U;
    if (hours > 0U) {
        std::snprintf(output, capacity, "%lu:%02lu:%02lu", static_cast<unsigned long>(hours), static_cast<unsigned long>(minutes),
                      static_cast<unsigned long>(seconds));
        return;
    }
    std::snprintf(output, capacity, "%lu:%02lu", static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
}

bool formatDateTimePattern(const DateTime& value, const char* pattern, char* output, const size_t capacity) {
    if (output == nullptr || capacity == 0U) {
        return false;
    }
    output[0] = '\0';
    if (pattern == nullptr) {
        return true;
    }

    size_t length = 0U;
    const char* cursor = pattern;
    while (*cursor != '\0') {
        if (*cursor == '[') {
            if (!consumeLiteralSpan(cursor, output, capacity, length)) {
                return false;
            }
            continue;
        }
        if (std::strncmp(cursor, "YYYY", 4) == 0) {
            if (!appendPadded(output, capacity, length, value.year(), 4)) {
                return false;
            }
            cursor += 4;
        } else if (std::strncmp(cursor, "EEEE", 4) == 0) {
            if (!appendString(output, capacity, length, kWeekdayLongNames[value.weekday() - 1U])) {
                return false;
            }
            cursor += 4;
        } else if (std::strncmp(cursor, "EEE", 3) == 0) {
            if (!appendString(output, capacity, length, kWeekdayShortNames[value.weekday() - 1U])) {
                return false;
            }
            cursor += 3;
        } else if (std::strncmp(cursor, "YY", 2) == 0) {
            if (!appendPadded(output, capacity, length, value.year() % 100U, 2)) {
                return false;
            }
            cursor += 2;
        } else if (std::strncmp(cursor, "MM", 2) == 0) {
            if (!appendPadded(output, capacity, length, value.month(), 2)) {
                return false;
            }
            cursor += 2;
        } else if (std::strncmp(cursor, "DD", 2) == 0) {
            if (!appendPadded(output, capacity, length, value.day(), 2)) {
                return false;
            }
            cursor += 2;
        } else if (std::strncmp(cursor, "HH", 2) == 0) {
            if (!appendPadded(output, capacity, length, value.hour(), 2)) {
                return false;
            }
            cursor += 2;
        } else if (std::strncmp(cursor, "mm", 2) == 0) {
            if (!appendPadded(output, capacity, length, value.minute(), 2)) {
                return false;
            }
            cursor += 2;
        } else if (std::strncmp(cursor, "ss", 2) == 0) {
            if (!appendPadded(output, capacity, length, value.second(), 2)) {
                return false;
            }
            cursor += 2;
        } else if (*cursor == 'M') {
            if (!appendNumber(output, capacity, length, value.month())) {
                return false;
            }
            ++cursor;
        } else if (*cursor == 'D') {
            if (!appendNumber(output, capacity, length, value.day())) {
                return false;
            }
            ++cursor;
        } else if (*cursor == 'H') {
            if (!appendNumber(output, capacity, length, value.hour())) {
                return false;
            }
            ++cursor;
        } else if (*cursor == 'm') {
            if (!appendNumber(output, capacity, length, value.minute())) {
                return false;
            }
            ++cursor;
        } else if (*cursor == 's') {
            if (!appendNumber(output, capacity, length, value.second())) {
                return false;
            }
            ++cursor;
        } else {
            if (!appendChar(output, capacity, length, *cursor)) {
                return false;
            }
            ++cursor;
        }
    }
    return true;
}

bool formatDurationPattern(const uint32_t durationMs, const char* pattern, char* output, const size_t capacity) {
    if (output == nullptr || capacity == 0U) {
        return false;
    }
    output[0] = '\0';
    if (pattern == nullptr) {
        return true;
    }

    const uint32_t totalSeconds = durationMs / 1000U;
    const uint32_t hours = totalSeconds / 3600U;
    const uint32_t minutes = (totalSeconds / 60U) % 60U;
    const uint32_t seconds = totalSeconds % 60U;

    size_t length = 0U;
    const char* cursor = pattern;
    while (*cursor != '\0') {
        if (*cursor == '[') {
            if (!consumeLiteralSpan(cursor, output, capacity, length)) {
                return false;
            }
            continue;
        }
        if (std::strncmp(cursor, "HH", 2) == 0) {
            if (!appendPadded(output, capacity, length, hours, 2)) {
                return false;
            }
            cursor += 2;
        } else if (std::strncmp(cursor, "mm", 2) == 0) {
            if (!appendPadded(output, capacity, length, minutes, 2)) {
                return false;
            }
            cursor += 2;
        } else if (std::strncmp(cursor, "ss", 2) == 0) {
            if (!appendPadded(output, capacity, length, seconds, 2)) {
                return false;
            }
            cursor += 2;
        } else if (*cursor == 'H') {
            if (!appendNumber(output, capacity, length, hours)) {
                return false;
            }
            ++cursor;
        } else if (*cursor == 'm') {
            if (!appendNumber(output, capacity, length, minutes)) {
                return false;
            }
            ++cursor;
        } else if (*cursor == 's') {
            if (!appendNumber(output, capacity, length, seconds)) {
                return false;
            }
            ++cursor;
        } else if (*cursor == 'Y' || *cursor == 'M' || *cursor == 'D' || *cursor == 'E') {
            return false; // date-only token used against a Duration value
        } else {
            if (!appendChar(output, capacity, length, *cursor)) {
                return false;
            }
            ++cursor;
        }
    }
    return true;
}

bool formatFixedDecimals(const MetricValue& value, const uint8_t digits, char* output, const size_t capacity) {
    if (output == nullptr || capacity == 0U) {
        return false;
    }
    output[0] = '\0';
    if (digits > 6U) {
        return false;
    }

    double numeric = 0.0;
    if (value.valueType == MetricValueType::Int || value.valueType == MetricValueType::Duration) {
        const auto* intValue = std::get_if<int32_t>(&value.number);
        if (intValue == nullptr) {
            return false;
        }
        numeric = static_cast<double>(*intValue);
    } else if (value.valueType == MetricValueType::Float) {
        const auto* floatValue = std::get_if<float>(&value.number);
        if (floatValue == nullptr || !std::isfinite(*floatValue)) {
            return false;
        }
        numeric = static_cast<double>(*floatValue);
    } else {
        return false;
    }

    char format[8];
    std::snprintf(format, sizeof(format), "%%.%uf", static_cast<unsigned>(digits));
    std::snprintf(output, capacity, format, numeric);
    return true;
}

bool metricValueHasNumericPreview(const MetricValue& value) {
    switch (value.valueType) {
    case MetricValueType::Int:
    case MetricValueType::Float:
    case MetricValueType::DateTime:
    case MetricValueType::Duration:
        return true;
    default:
        return false;
    }
}

double metricValueNumericPreview(const MetricValue& value) {
    if (const auto* intValue = std::get_if<int32_t>(&value.number); intValue != nullptr) {
        return static_cast<double>(*intValue);
    }
    if (const auto* floatValue = std::get_if<float>(&value.number); floatValue != nullptr) {
        return static_cast<double>(*floatValue);
    }
    if (const auto* dateTimeValue = std::get_if<DateTime>(&value.number); dateTimeValue != nullptr) {
        return static_cast<double>(dateTimeValue->unixtime());
    }
    return 0.0;
}

} // namespace ewfm
