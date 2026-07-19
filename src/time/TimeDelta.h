#pragma once

#include <cstdint>

namespace ewfm {

// A signed duration in whole seconds, decomposable into days/hours/minutes/seconds-of-day.
class TimeDelta {
public:
    explicit TimeDelta(int32_t seconds = 0) : seconds_(seconds) {}
    TimeDelta(int16_t days, int8_t hours, int8_t minutes, int8_t seconds) : seconds_(calcTotalSeconds(days, hours, minutes, seconds)) {}

    static int32_t calcTotalSeconds(int16_t days, int8_t hours, int8_t minutes, int8_t seconds);

    [[nodiscard]] int16_t days() const;
    [[nodiscard]] int8_t hours() const;
    [[nodiscard]] int8_t minutes() const;
    [[nodiscard]] int8_t seconds() const;
    [[nodiscard]] int32_t totalSeconds() const {
        return seconds_;
    }

    TimeDelta operator+(const TimeDelta& right) const {
        return TimeDelta(seconds_ + right.seconds_);
    }
    TimeDelta operator-(const TimeDelta& right) const {
        return TimeDelta(seconds_ - right.seconds_);
    }
    bool operator==(const TimeDelta& right) const {
        return seconds_ == right.seconds_;
    }
    bool operator!=(const TimeDelta& right) const {
        return !(*this == right);
    }

private:
    [[nodiscard]] int32_t secondsOfDay() const;

    int32_t seconds_;
};

} // namespace ewfm
