#include "time/Iso8601.h"

#include <cstdio>

namespace ewfm {

namespace {

bool parseDigits(const char*& p, const char* end, int count, int& out) {
    if (end - p < count) {
        return false;
    }
    out = 0;
    for (int i = 0; i < count; ++i) {
        if (p[i] < '0' || p[i] > '9') {
            return false;
        }
        out = out * 10 + (p[i] - '0');
    }
    p += count;
    return true;
}

} // namespace

std::optional<DateTime> parseIso8601(const char* str, size_t len) {
    if (str == nullptr) {
        return std::nullopt;
    }
    const char* p = str;
    const char* end = str + len;

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (!parseDigits(p, end, 4, year)) {
        return std::nullopt;
    }
    if (p == end || *p != '-') {
        return std::nullopt;
    }
    ++p;
    if (!parseDigits(p, end, 2, month) || month < 1 || month > 12) {
        return std::nullopt;
    }
    if (p == end || *p != '-') {
        return std::nullopt;
    }
    ++p;
    if (!parseDigits(p, end, 2, day) || day < 1 || day > 31) {
        return std::nullopt;
    }

    if (p == end || (*p != 'T' && *p != ' ')) {
        return std::nullopt;
    }
    ++p;

    if (!parseDigits(p, end, 2, hour) || hour >= 24) {
        return std::nullopt;
    }
    if (p == end || *p != ':') {
        return std::nullopt;
    }
    ++p;
    if (!parseDigits(p, end, 2, minute) || minute >= 60) {
        return std::nullopt;
    }
    if (p != end && *p == ':') {
        ++p;
        if (!parseDigits(p, end, 2, second) || second >= 60) {
            return std::nullopt;
        }
    }

    TimeDelta offset(0);
    if (p != end) {
        if (*p == 'Z') {
            ++p;
        } else if (*p == '+' || *p == '-') {
            const int sign = (*p == '-') ? -1 : 1;
            ++p;
            int tzHour = 0;
            int tzMinute = 0;
            if (!parseDigits(p, end, 2, tzHour) || tzHour >= 24) {
                return std::nullopt;
            }
            if (p != end && *p == ':') {
                ++p;
            }
            if (p != end) {
                if (!parseDigits(p, end, 2, tzMinute) || tzMinute >= 60) {
                    return std::nullopt;
                }
            }
            offset = TimeDelta(0, static_cast<int8_t>(tzHour * sign), static_cast<int8_t>(tzMinute * sign), 0);
        } else {
            return std::nullopt;
        }
    }
    if (p != end) {
        return std::nullopt;
    }

    return DateTime(static_cast<uint16_t>(year), static_cast<uint8_t>(month), static_cast<uint8_t>(day), static_cast<uint8_t>(hour),
                    static_cast<uint8_t>(minute), static_cast<uint8_t>(second), TimeZoneInfo(offset));
}

void formatIso8601(const DateTime& dt, char* buf, size_t bufSize) {
    const int32_t offsetSeconds = dt.tzInfo().offsetSeconds();
    if (offsetSeconds == 0) {
        std::snprintf(buf, bufSize, "%04u-%02u-%02uT%02u:%02u:%02uZ", static_cast<unsigned>(dt.year()), static_cast<unsigned>(dt.month()),
                      static_cast<unsigned>(dt.day()), static_cast<unsigned>(dt.hour()), static_cast<unsigned>(dt.minute()),
                      static_cast<unsigned>(dt.second()));
        return;
    }
    const int32_t absOffsetMinutes = (offsetSeconds < 0 ? -offsetSeconds : offsetSeconds) / 60;
    std::snprintf(buf, bufSize, "%04u-%02u-%02uT%02u:%02u:%02u%c%02d:%02d", static_cast<unsigned>(dt.year()),
                  static_cast<unsigned>(dt.month()), static_cast<unsigned>(dt.day()), static_cast<unsigned>(dt.hour()),
                  static_cast<unsigned>(dt.minute()), static_cast<unsigned>(dt.second()), offsetSeconds < 0 ? '-' : '+',
                  static_cast<int>(absOffsetMinutes / 60), static_cast<int>(absOffsetMinutes % 60));
}

} // namespace ewfm
