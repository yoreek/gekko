#include "time/DateTime.h"

namespace ewfm {

namespace {
constexpr uint32_t kSecondsPerMinute = 60UL;
constexpr uint32_t kSecondsPerHour = 3600UL;
constexpr uint32_t kSecondsPerDay = 86400UL;
constexpr uint8_t kMonthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Mirrors TimeLib's LEAP_YEAR(Y) macro, where Y is a year offset from 1970.
bool isLeapYear(uint16_t yearsSince1970) {
    const uint32_t year = 1970U + yearsSince1970;
    return year % 4U == 0U && (year % 100U != 0U || year % 400U == 0U);
}
} // namespace

DateTime::DateTime(uint32_t epochSeconds, TimeZoneInfo tzInfo) : time_(epochSeconds), tzInfo_(tzInfo) {}

DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, TimeZoneInfo tzInfo)
    : time_(assemble(year, month, day, hour, minute, second)), tzInfo_(tzInfo) {}

// Ported from TimeLib's breakTime(), decoupled from its global sync engine so this stays
// dependency-free and native-testable.
DateTime::CalendarFields DateTime::breakDown(uint32_t epochSeconds) {
    CalendarFields fields{};
    uint32_t time = epochSeconds;
    fields.second = static_cast<uint8_t>(time % 60U);
    time /= 60U;
    fields.minute = static_cast<uint8_t>(time % 60U);
    time /= 60U;
    fields.hour = static_cast<uint8_t>(time % 24U);
    time /= 24U;                                                    // now days since epoch
    fields.weekday = static_cast<uint8_t>(((time + 4U) % 7U) + 1U); // Sunday is day 1

    uint16_t yearsSince1970 = 0;
    uint32_t daysAccumulated = 0;
    while (daysAccumulated + (isLeapYear(yearsSince1970) ? 366U : 365U) <= time) {
        daysAccumulated += isLeapYear(yearsSince1970) ? 366U : 365U;
        ++yearsSince1970;
    }
    fields.year = static_cast<uint16_t>(1970U + yearsSince1970);

    uint32_t dayOfYear = time - daysAccumulated;
    uint8_t month = 0;
    for (; month < 12U; ++month) {
        const uint8_t monthLength = (month == 1U && isLeapYear(yearsSince1970)) ? 29U : kMonthDays[month];
        if (dayOfYear >= monthLength) {
            dayOfYear -= monthLength;
        } else {
            break;
        }
    }
    fields.month = static_cast<uint8_t>(month + 1U);
    fields.day = static_cast<uint8_t>(dayOfYear + 1U);
    return fields;
}

// Ported from TimeLib's makeTime().
uint32_t DateTime::assemble(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    const uint16_t yearsSince1970 = static_cast<uint16_t>(year - 1970U);
    uint32_t days = 0;
    for (uint16_t y = 0; y < yearsSince1970; ++y) {
        days += isLeapYear(y) ? 366U : 365U;
    }
    for (uint8_t m = 0; static_cast<uint8_t>(m + 1U) < month; ++m) {
        days += (m == 1U && isLeapYear(yearsSince1970)) ? 29U : kMonthDays[m];
    }
    days += static_cast<uint32_t>(day - 1U);
    return days * kSecondsPerDay + static_cast<uint32_t>(hour) * kSecondsPerHour + static_cast<uint32_t>(minute) * kSecondsPerMinute +
           second;
}

uint16_t DateTime::year() const {
    return breakDown(time_).year;
}
uint8_t DateTime::month() const {
    return breakDown(time_).month;
}
uint8_t DateTime::day() const {
    return breakDown(time_).day;
}
uint8_t DateTime::hour() const {
    return breakDown(time_).hour;
}
uint8_t DateTime::minute() const {
    return breakDown(time_).minute;
}
uint8_t DateTime::second() const {
    return breakDown(time_).second;
}
uint8_t DateTime::weekday() const {
    return breakDown(time_).weekday;
}

} // namespace ewfm
