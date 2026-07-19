#include "devices/dosing/DosingPumpSchedule.h"

namespace ewfm {

namespace {
constexpr uint32_t kSecondsPerDay = 86400UL;
// Covers the largest everyDays cycle (30) with margin; weekly needs only 7.
constexpr uint32_t kNextDoseScanDays = 2UL * kDosingPumpMaxEveryDays + 1UL;

// Day 0 (1970-01-01) was a Thursday; 0=Sunday..6=Saturday to match daysOfWeekMask.
uint8_t weekdayIndexOfDay(uint32_t localDayNumber) {
    return static_cast<uint8_t>((localDayNumber + 4UL) % 7UL);
}
} // namespace

bool dosingDayActive(const DosingPumpDeviceConfigV1& config, uint32_t localDayNumber) {
    if (static_cast<DosingScheduleMode>(config.scheduleMode) == DosingScheduleMode::Weekly) {
        return (config.daysOfWeekMask & (1U << weekdayIndexOfDay(localDayNumber))) != 0U;
    }
    if (config.everyDays <= 1U) {
        return true;
    }
    // A browser-clock anchor can land a day ahead of the device's local day - the cycle phase is
    // well-defined in both directions.
    const uint32_t distance =
        localDayNumber >= config.anchorDay ? localDayNumber - config.anchorDay : static_cast<uint32_t>(config.anchorDay) - localDayNumber;
    return distance % config.everyDays == 0U;
}

int dueDoseIndex(const DosingPumpDeviceConfigV1& config, uint16_t nowMinuteOfDay, uint16_t firedMask) {
    for (uint8_t index = 0U; index < config.doseCount; ++index) {
        if ((firedMask & (1U << index)) != 0U) {
            continue;
        }
        const uint16_t doseMinute = config.doses[index].minuteOfDay;
        if (nowMinuteOfDay >= doseMinute && nowMinuteOfDay < doseMinute + kDosingPumpGraceMinutes) {
            return index;
        }
    }
    return -1;
}

uint16_t missedDosesMask(const DosingPumpDeviceConfigV1& config, uint16_t nowMinuteOfDay) {
    uint16_t mask = 0U;
    for (uint8_t index = 0U; index < config.doseCount; ++index) {
        if (nowMinuteOfDay >= config.doses[index].minuteOfDay + kDosingPumpGraceMinutes) {
            mask = static_cast<uint16_t>(mask | (1U << index));
        }
    }
    return mask;
}

bool nextDose(const DosingPumpDeviceConfigV1& config, uint32_t localDayNumber, uint16_t nowMinuteOfDay, uint16_t firedMask,
              uint16_t skipNextMask, uint32_t& outEpoch, uint16_t& outAmountCentiMl) {
    if (config.doseCount == 0U) {
        return false;
    }
    // Each skipNext bit suppresses the first occurrence of its dose encountered in chronological
    // order - the same order the runtime consumes them in.
    uint16_t pendingSkips = skipNextMask;
    for (uint32_t dayOffset = 0U; dayOffset < kNextDoseScanDays; ++dayOffset) {
        const uint32_t day = localDayNumber + dayOffset;
        if (!dosingDayActive(config, day)) {
            continue;
        }
        for (uint8_t index = 0U; index < config.doseCount; ++index) {
            const uint16_t doseBit = static_cast<uint16_t>(1U << index);
            if (dayOffset == 0U) {
                if (config.doses[index].minuteOfDay < nowMinuteOfDay) {
                    continue;
                }
                if ((firedMask & doseBit) != 0U) {
                    continue;
                }
            }
            if ((pendingSkips & doseBit) != 0U) {
                pendingSkips = static_cast<uint16_t>(pendingSkips & ~doseBit);
                continue;
            }
            outEpoch = day * kSecondsPerDay + static_cast<uint32_t>(config.doses[index].minuteOfDay) * 60UL;
            outAmountCentiMl = config.doses[index].amountCentiMl;
            return true;
        }
    }
    return false;
}

uint32_t todayTargetCentiMl(const DosingPumpDeviceConfigV1& config, bool dayActive) {
    if (!dayActive) {
        return 0U;
    }
    uint32_t total = 0U;
    for (uint8_t index = 0U; index < config.doseCount; ++index) {
        total += config.doses[index].amountCentiMl;
    }
    return total;
}

uint32_t averageDailyCentiMl(const DosingPumpDeviceConfigV1& config) {
    const uint32_t totalPerActiveDay = todayTargetCentiMl(config, true);
    if (totalPerActiveDay == 0U) {
        return 0U;
    }
    if (static_cast<DosingScheduleMode>(config.scheduleMode) == DosingScheduleMode::Weekly) {
        uint32_t activeDays = 0U;
        for (uint8_t day = 0U; day < 7U; ++day) {
            if ((config.daysOfWeekMask & (1U << day)) != 0U) {
                ++activeDays;
            }
        }
        return totalPerActiveDay * activeDays / 7U;
    }
    const uint8_t everyDays = config.everyDays > 0U ? config.everyDays : 1U;
    return totalPerActiveDay / everyDays;
}

uint32_t doseRunMs(uint16_t amountCentiMl, uint16_t speedMilliMlPerSec) {
    if (speedMilliMlPerSec == 0U) {
        return 0U;
    }
    // amount[0.01 ml] * 10 -> microliters; / speed[ul/s] -> seconds; * 1000 -> ms.
    return static_cast<uint32_t>(static_cast<uint64_t>(amountCentiMl) * 10000ULL / speedMilliMlPerSec);
}

uint32_t dosedCentiMl(uint32_t elapsedMs, uint16_t speedMilliMlPerSec) {
    return static_cast<uint32_t>(static_cast<uint64_t>(elapsedMs) * speedMilliMlPerSec / 10000ULL);
}

} // namespace ewfm
