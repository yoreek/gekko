#pragma once

#include <cstddef>
#include <cstdint>

namespace ewfm {

// POD mirror of jchristensen/Timezone's TimeChangeRule (identical field layout/meaning), so this
// file has zero dependency on <Timezone.h>/<Arduino.h> and stays native-buildable/testable. Only
// DateTime::applyTimezone() (in DateTimeTimezone.cpp, Arduino-only) knows how to turn this into a
// real TimeChangeRule.
struct TimeZoneChangeRule {
    const char abbrev[6];
    int8_t week;           // 0=Last, 1=First, 2=Second, 3=Third, 4=Fourth week of the month
    int8_t dow;            // day of week, 1=Sun, 2=Mon, ... 7=Sat
    int8_t month;          // 1=Jan, 2=Feb, ... 12=Dec
    int8_t hour;           // 0-23
    int16_t offsetMinutes; // offset from UTC in minutes
};

struct TimeZoneEntry {
    const char* id;
    TimeZoneChangeRule dst;
    TimeZoneChangeRule std;
};

// Ported verbatim (same ids/order/DST rules) from a previous project's src/time/TimeZoneUtil.cpp.
// The human-readable labels are not stored here - they live in the SPA (test/test_core/timezones.json /
// portal-spa timezone catalog), the only consumer of them. Keep the id set/order in sync with that
// list.
extern const TimeZoneEntry kTimeZoneTable[];
extern const size_t kTimeZoneTableCount;

// Linear lookup by IANA-style id (e.g. "America/New_York"); returns nullptr if not found.
const TimeZoneEntry* findTimeZoneEntry(const char* id);

} // namespace ewfm
