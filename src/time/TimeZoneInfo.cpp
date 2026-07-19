#include "time/TimeZoneInfo.h"

#include <cstdio>
#include <cstdlib>

namespace ewfm {

TimeZoneInfo::TimeZoneInfo(const char* name, TimeDelta offset) : offset_(offset) {
    std::snprintf(name_, sizeof(name_), "%s", name);
}

TimeZoneInfo::TimeZoneInfo(TimeDelta offset) : offset_(offset) {
    std::snprintf(name_, sizeof(name_), "UTC%c%02d:%02d", offset.totalSeconds() < 0 ? '-' : '+', std::abs(offset.hours()),
                  std::abs(offset.minutes()));
}

} // namespace ewfm
