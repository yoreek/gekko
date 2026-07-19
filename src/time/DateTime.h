#pragma once

#include "time/TimeDelta.h"
#include "time/TimeZoneInfo.h"

#include <cstdint>

namespace ewfm {

// A calendar timestamp: an epoch-like seconds count paired with the timezone info used to
// annotate it. `unixtime()` returns whatever "flavor" of epoch the caller constructed it with
// (UTC seconds when tzInfo is UTC, or local wall-clock seconds when tzInfo carries a non-zero
// offset) - `utcUnixtime()`/`toUtc()` always normalize back to true UTC seconds.
//
// Calendar field extraction is self-contained arithmetic (no <TimeLib.h>/<Arduino.h> dependency),
// so this type builds and unit-tests under env:native like the rest of the codebase.
class DateTime {
public:
    // Epoch 0 (1970-01-01 UTC) - year() is well below any kMinValidYear-style sentinel used by
    // callers to detect "wall clock not set yet", so a default-constructed DateTime reads as
    // invalid/unset without any extra sentinel field.
    DateTime() : DateTime(0, TimeZoneInfo::utc()) {}
    explicit DateTime(uint32_t epochSeconds, TimeZoneInfo tzInfo = TimeZoneInfo::utc());
    DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0,
             TimeZoneInfo tzInfo = TimeZoneInfo::utc());

#if defined(ARDUINO) && !defined(UNIT_TEST)
    // Resolves timezoneId's rules and applies them as the app's current timezone (see
    // DateTimeTimezone.cpp, the only file allowed to depend on <Timezone.h>). Call this only when
    // the configured zone actually changes (ConfigStore::save()) - NOT on every current() call.
    // Returns false (rules left unchanged) if timezoneId isn't in kTimeZoneTable.
    static bool applyTimezone(const char* timezoneId);

    // Current local time, using whatever timezone was last applied via applyTimezone(). The only
    // DateTime method that needs the Arduino Timezone library - every other member here stays
    // plain calendar arithmetic and portable/native-testable.
    static DateTime current();
#endif

    [[nodiscard]] uint16_t year() const;
    [[nodiscard]] uint8_t month() const;
    [[nodiscard]] uint8_t day() const;
    [[nodiscard]] uint8_t hour() const;
    [[nodiscard]] uint8_t minute() const;
    [[nodiscard]] uint8_t second() const;
    [[nodiscard]] uint8_t weekday() const; // Sunday is day 1

    [[nodiscard]] uint32_t unixtime() const {
        return time_;
    }
    [[nodiscard]] uint32_t utcUnixtime() const {
        return time_ - static_cast<uint32_t>(tzInfo_.offsetSeconds());
    }
    [[nodiscard]] DateTime toUtc() const {
        return DateTime(utcUnixtime(), TimeZoneInfo::utc());
    }
    [[nodiscard]] const TimeZoneInfo& tzInfo() const {
        return tzInfo_;
    }

    [[nodiscard]] uint32_t secondsOfDay() const {
        return time_ % kSecondsPerDay;
    }
    [[nodiscard]] uint16_t minutesOfDay() const {
        return static_cast<uint16_t>(secondsOfDay() / 60U);
    }

    bool operator<(const DateTime& right) const {
        return utcUnixtime() < right.utcUnixtime();
    }
    bool operator>(const DateTime& right) const {
        return right < *this;
    }
    bool operator<=(const DateTime& right) const {
        return !(*this > right);
    }
    bool operator>=(const DateTime& right) const {
        return !(*this < right);
    }
    bool operator==(const DateTime& right) const {
        return utcUnixtime() == right.utcUnixtime();
    }
    bool operator!=(const DateTime& right) const {
        return !(*this == right);
    }

    DateTime operator+(const TimeDelta& delta) const {
        return DateTime(time_ + static_cast<uint32_t>(delta.totalSeconds()), tzInfo_);
    }
    DateTime operator-(const TimeDelta& delta) const {
        return DateTime(time_ - static_cast<uint32_t>(delta.totalSeconds()), tzInfo_);
    }
    TimeDelta operator-(const DateTime& right) const {
        return TimeDelta(static_cast<int32_t>(utcUnixtime() - right.utcUnixtime()));
    }

private:
    static constexpr uint32_t kSecondsPerDay = 86400UL;

    struct CalendarFields {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
        uint8_t weekday;
    };
    static CalendarFields breakDown(uint32_t epochSeconds);
    static uint32_t assemble(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

    uint32_t time_;
    TimeZoneInfo tzInfo_;
};

} // namespace ewfm
