#pragma once

#include "time/TimeDelta.h"

namespace ewfm {

// The resolved name + UTC offset for a DateTime instant (e.g. "EST" at -05:00 for a
// DST-observing zone at a specific moment, or a synthesized "UTC+02:00" label for a fixed
// offset). Small and trivially copyable so it can be embedded by value inside DateTime.
class TimeZoneInfo {
public:
    TimeZoneInfo() : TimeZoneInfo("UTC", TimeDelta(0)) {}
    TimeZoneInfo(const char* name, TimeDelta offset);
    explicit TimeZoneInfo(TimeDelta offset);

    [[nodiscard]] const char* name() const {
        return name_;
    }
    [[nodiscard]] TimeDelta offset() const {
        return offset_;
    }
    [[nodiscard]] int32_t offsetSeconds() const {
        return offset_.totalSeconds();
    }

    static TimeZoneInfo utc() {
        return TimeZoneInfo("UTC", TimeDelta(0));
    }

private:
    char name_[10]{};
    TimeDelta offset_;
};

} // namespace ewfm
