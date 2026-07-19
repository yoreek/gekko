#pragma once

#include "devices/dosing/DosingPumpDeviceConfig.h"

#include <cstdint>

namespace ewfm {

// Pure dose-schedule/run-time arithmetic with no wall-clock or Arduino dependency - the
// native-testable core the DosingPumpDevice runtime delegates to (mirrors the
// ScheduleDevice::isActiveAt() split). All "day" parameters are local days since 1970
// (local-flavored unixtime()/86400, same flavor DateTime::current() carries).

// Whether doses fire at all on the given day: daily mode follows the everyDays cycle phase
// anchored at anchorDay, weekly mode follows daysOfWeekMask.
bool dosingDayActive(const DosingPumpDeviceConfigV1& config, uint32_t localDayNumber);

// First unfired dose whose grace window [minuteOfDay, minuteOfDay + kDosingPumpGraceMinutes) covers
// nowMinuteOfDay; -1 when none is due.
int dueDoseIndex(const DosingPumpDeviceConfigV1& config, uint16_t nowMinuteOfDay, uint16_t firedMask);

// Doses whose grace window has already fully passed by nowMinuteOfDay - the "drop, don't dose
// late" mask ORed into firedMask on boot catch-up and busy overruns.
uint16_t missedDosesMask(const DosingPumpDeviceConfigV1& config, uint16_t nowMinuteOfDay);

// Next dose occurrence that will actually run, scanning from (localDayNumber, nowMinuteOfDay)
// forward. firedMask applies to the current day only; each skipNext bit suppresses the first
// occurrence of its dose encountered in the scan (matching how the runtime consumes skip bits).
// Returns false when the schedule is empty or no active day exists in the scan horizon.
// outEpoch is a local-flavored epoch (localDay * 86400 + minute * 60).
bool nextDose(const DosingPumpDeviceConfigV1& config, uint32_t localDayNumber, uint16_t nowMinuteOfDay, uint16_t firedMask,
              uint16_t skipNextMask, uint32_t& outEpoch, uint16_t& outAmountCentiMl);

// Sum of all scheduled dose amounts for a day the schedule is active on (0 when dayActive is
// false).
uint32_t todayTargetCentiMl(const DosingPumpDeviceConfigV1& config, bool dayActive);

// Long-run average consumption per day (total per active day spread over the cycle), used for the
// days-left estimate. 0 when the schedule is empty.
uint32_t averageDailyCentiMl(const DosingPumpDeviceConfigV1& config);

// amount / speed as a millisecond run duration: amountCentiMl is 0.01 ml, speed is 0.001 ml/s.
uint32_t doseRunMs(uint16_t amountCentiMl, uint16_t speedMilliMlPerSec);

// Inverse of doseRunMs for early stop: how much was dispensed after elapsedMs of running.
uint32_t dosedCentiMl(uint32_t elapsedMs, uint16_t speedMilliMlPerSec);

} // namespace ewfm
