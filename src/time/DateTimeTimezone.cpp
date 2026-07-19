// The only file in src/time/ allowed to depend on <Timezone.h> (jchristensen/Timezone). Implements
// DateTime::applyTimezone()/current() (declared in DateTime.h, guarded there the same way) so the
// rest of DateTime stays plain calendar arithmetic and portable/native-testable.
#include "time/DateTime.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)

#include "time/TimeZoneTable.h"

#include <TimeLib.h>
#include <Timezone.h>
#include <cstring>

namespace ewfm {

namespace {
TimeChangeRule toTimeChangeRule(const TimeZoneChangeRule& rule) {
    TimeChangeRule result{};
    std::strncpy(result.abbrev, rule.abbrev, sizeof(result.abbrev) - 1);
    result.week = rule.week;
    result.dow = rule.dow;
    result.month = rule.month;
    result.hour = rule.hour;
    result.offset = rule.offsetMinutes;
    return result;
}

const TimeChangeRule kBootRule{"UTC", 1, 1, 1, 0, 0};

// One persistent Timezone object (mirrors the reference design's global `systemTimezone`),
// rewritten in place by applyTimezone() only when the configured zone actually changes. Guarded
// because it's written from ConfigStore::save() and read from current() on both the main loop
// (ScheduleDevice::isActive(), via App::tick()) and the async_tcp task (TimeController's HTTP
// handler).
portMUX_TYPE g_timezoneMux = portMUX_INITIALIZER_UNLOCKED;
Timezone g_systemTimezone(kBootRule);
} // namespace

bool DateTime::applyTimezone(const char* timezoneId) {
    const TimeZoneEntry* entry = findTimeZoneEntry(timezoneId);
    if (entry == nullptr) {
        return false;
    }
    portENTER_CRITICAL(&g_timezoneMux);
    g_systemTimezone.setRules(toTimeChangeRule(entry->dst), toTimeChangeRule(entry->std));
    portEXIT_CRITICAL(&g_timezoneMux);
    return true;
}

DateTime DateTime::current() {
    TimeChangeRule* activeRule = nullptr;
    time_t localEpoch = 0;
    int32_t offsetSeconds = 0;
    char abbrev[6]{};

    portENTER_CRITICAL(&g_timezoneMux);
    localEpoch = g_systemTimezone.toLocal(now(), &activeRule);
    offsetSeconds = activeRule != nullptr ? static_cast<int32_t>(activeRule->offset) * 60 : 0;
    // Copy the abbreviation out while still locked - activeRule points into g_systemTimezone,
    // which applyTimezone() could rewrite the instant the critical section is released.
    std::strncpy(abbrev, activeRule != nullptr ? activeRule->abbrev : "UTC", sizeof(abbrev) - 1);
    portEXIT_CRITICAL(&g_timezoneMux);

    const TimeZoneInfo tzInfo(abbrev, TimeDelta(offsetSeconds));
    return DateTime(static_cast<uint32_t>(localEpoch), tzInfo);
}

} // namespace ewfm

#endif
