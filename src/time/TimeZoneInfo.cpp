#include "time/TimeZoneInfo.h"

#include <cstdio>
#include <cstdlib>

namespace ewfm {

TimeZoneInfo::TimeZoneInfo(const char* name, TimeDelta offset) : offset_(offset) {
    std::snprintf(name_, sizeof(name_), "%s", name);
}

TimeZoneInfo::TimeZoneInfo(TimeDelta offset) : offset_(offset) {
    const int hours = std::abs(static_cast<int>(offset.hours())) % 24;
    const int minutes = std::abs(static_cast<int>(offset.minutes())) % 60;
    std::snprintf(name_, sizeof(name_), "UTC%c%02d:%02d", offset.totalSeconds() < 0 ? '-' : '+', hours, minutes);
}

} // namespace ewfm
