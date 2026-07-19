#include "time/TimeDelta.h"

namespace ewfm {

namespace {
constexpr int32_t kSecondsPerMinute = 60;
constexpr int32_t kSecondsPerHour = 3600;
constexpr int32_t kSecondsPerDay = 86400;
} // namespace

int32_t TimeDelta::calcTotalSeconds(int16_t days, int8_t hours, int8_t minutes, int8_t seconds) {
    return static_cast<int32_t>(days) * kSecondsPerDay + static_cast<int32_t>(hours) * kSecondsPerHour +
           static_cast<int32_t>(minutes) * kSecondsPerMinute + static_cast<int32_t>(seconds);
}

int16_t TimeDelta::days() const {
    auto value = static_cast<int16_t>(seconds_ / kSecondsPerDay);
    if (value < 0 && (seconds_ % kSecondsPerDay) != 0) {
        --value;
    }
    return value;
}

int32_t TimeDelta::secondsOfDay() const {
    return seconds_ - static_cast<int32_t>(days()) * kSecondsPerDay;
}

int8_t TimeDelta::hours() const {
    return static_cast<int8_t>(secondsOfDay() / kSecondsPerHour % 24);
}

int8_t TimeDelta::minutes() const {
    return static_cast<int8_t>(secondsOfDay() / kSecondsPerMinute % 60);
}

int8_t TimeDelta::seconds() const {
    return static_cast<int8_t>(secondsOfDay() % 60);
}

} // namespace ewfm
