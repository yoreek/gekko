import type { ScheduleRuleConfig } from '@/api/contracts'

// Client-side mirror of ScheduleDevice::isActiveAt() (src/devices/schedule/ScheduleDevice.cpp) -
// the firmware never pushes a live "active" value over REST/WS (ScheduleDevice never marks itself
// runtime-dirty, so it would just be a stale snapshot from whenever the page last loaded). Instead
// the frontend recomputes on/off state directly from the rule config, using the *browser's* local
// clock - this is an approximation of the device's own configured-timezone clock, not the
// authoritative value AutoSwitchDevice actually acts on.

const MINUTES_PER_DAY = 1440

function isSuitableDayOfWeek(rule: ScheduleRuleConfig, weekday: number): boolean {
  return rule.weekDays.includes(weekday)
}

function isSuitableTimeRange(rule: ScheduleRuleConfig, minuteOfDay: number): boolean {
  const { startMinuteOfDay: start, endMinuteOfDay: end } = rule
  if (start === end) {
    return true
  }
  if (start < end) {
    return minuteOfDay >= start && minuteOfDay < end
  }
  return minuteOfDay >= start || minuteOfDay < end
}

function windowMinutes(rule: ScheduleRuleConfig): number {
  const { startMinuteOfDay: start, endMinuteOfDay: end } = rule
  if (start === end) {
    return MINUTES_PER_DAY
  }
  if (start < end) {
    return end - start
  }
  return MINUTES_PER_DAY - start + end
}

function isSuitableInterval(rule: ScheduleRuleConfig, minuteOfDay: number): boolean {
  if (rule.intervalsPerWindow <= 0) {
    return false
  }
  const intervalMinutes = Math.floor(windowMinutes(rule) / rule.intervalsPerWindow)
  if (intervalMinutes <= 0) {
    return false
  }
  const minute = minuteOfDay < rule.startMinuteOfDay ? minuteOfDay + MINUTES_PER_DAY : minuteOfDay
  const positionInInterval = (minute - rule.startMinuteOfDay) % intervalMinutes
  return positionInInterval < rule.durationMinutes
}

export interface IntervalSegmentPreview {
  sliceStartMinuteOfDay: number
  onEndMinuteOfDay: number
  sliceMinutes: number
}

// Every equal-length slice an Interval-mode rule cuts its window into - mirrors the same
// windowMinutes()/intervalsPerWindow slicing isSuitableInterval() checks against, just listing
// every slice instead of testing one instant. Each slice's "on" sub-range is clipped to the slice
// length (durationMinutes can be configured longer than a slice, which isSuitableInterval() then
// reads as "always on" for that slice - see intervalDurationExceedsSlice() below for the
// misconfiguration check, which deliberately ignores the trailing partial slice). If
// windowMinutes(rule) doesn't divide evenly by intervalsPerWindow, one trailing
// partial slice is appended for the remainder - isSuitableInterval()'s unbounded `% intervalMinutes`
// really does treat that remainder as one more short "on" segment, so this mirrors real firmware
// behavior rather than silently dropping it. Weekday-independent: the same slicing applies on
// every day this rule's weekDays selects. Empty for AlwaysOn rules or invalid config (matches
// isSuitableInterval()'s own guards: intervalsPerWindow <= 0 or the resulting intervalMinutes <= 0).
export function intervalSegmentStarts(rule: ScheduleRuleConfig): IntervalSegmentPreview[] {
  if (rule.mode !== 'interval' || rule.intervalsPerWindow <= 0) {
    return []
  }
  const totalMinutes = windowMinutes(rule)
  const intervalMinutes = Math.floor(totalMinutes / rule.intervalsPerWindow)
  if (intervalMinutes <= 0) {
    return []
  }
  const segments: IntervalSegmentPreview[] = Array.from({ length: rule.intervalsPerWindow }, (_, index) => {
    const sliceStartMinuteOfDay = (rule.startMinuteOfDay + index * intervalMinutes) % MINUTES_PER_DAY
    const onMinutes = Math.min(rule.durationMinutes, intervalMinutes)
    return {
      sliceStartMinuteOfDay,
      onEndMinuteOfDay: (sliceStartMinuteOfDay + onMinutes) % MINUTES_PER_DAY,
      sliceMinutes: intervalMinutes,
    }
  })
  const remainderMinutes = totalMinutes - rule.intervalsPerWindow * intervalMinutes
  if (remainderMinutes > 0) {
    const sliceStartMinuteOfDay = (rule.startMinuteOfDay + rule.intervalsPerWindow * intervalMinutes) % MINUTES_PER_DAY
    const onMinutes = Math.min(rule.durationMinutes, remainderMinutes)
    segments.push({
      sliceStartMinuteOfDay,
      onEndMinuteOfDay: (sliceStartMinuteOfDay + onMinutes) % MINUTES_PER_DAY,
      sliceMinutes: remainderMinutes,
    })
  }
  return segments
}

// Whether the configured on-duration is longer than a *regular* interval slice - the real
// misconfiguration case, where the rule stays on for the whole window instead of duty-cycling.
// Deliberately checks against the regular slice length (windowMinutes/intervalsPerWindow), not any
// individual segment's sliceMinutes: the trailing partial slice intervalSegmentStarts() appends for
// a non-evenly-dividing window is often shorter than every regular slice, so comparing durationMinutes
// against *that* would flag ordinary, correctly duty-cycling configurations (e.g. 7 intervals of 7
// minutes each over a 720-minute window leaves a 6-minute remainder, which is shorter than 7 - that
// remainder being fully "on" is an inconsequential rounding artifact, not a misconfiguration).
export function intervalDurationExceedsSlice(rule: ScheduleRuleConfig): boolean {
  if (rule.mode !== 'interval' || rule.intervalsPerWindow <= 0) {
    return false
  }
  const intervalMinutes = Math.floor(windowMinutes(rule) / rule.intervalsPerWindow)
  return intervalMinutes > 0 && rule.durationMinutes > intervalMinutes
}

export function isRuleActive(rule: ScheduleRuleConfig, weekday: number, minuteOfDay: number): boolean {
  if (!rule.enabled) {
    return false
  }
  if (!isSuitableDayOfWeek(rule, weekday) || !isSuitableTimeRange(rule, minuteOfDay)) {
    return false
  }
  if (rule.mode === 'interval') {
    return isSuitableInterval(rule, minuteOfDay)
  }
  return true
}

// Date.getDay() (0=Sunday..6=Saturday) matches this app's weekDays bit convention directly - no
// conversion needed.
export function isScheduleActiveAt(rules: ScheduleRuleConfig[], at: Date): boolean {
  const weekday = at.getDay()
  const minuteOfDay = at.getHours() * 60 + at.getMinutes()
  return rules.some(rule => isRuleActive(rule, weekday, minuteOfDay))
}

export interface ScheduleStatusPreview {
  active: boolean
  nextChangeAt: Date | null
}

// Scans forward minute-by-minute (bounded by horizonMinutes, default one week) for the next moment
// isScheduleActiveAt()'s result flips. Cheap enough for a one-shot UI computation (<=4 rules, <=
// 10080 iterations) - not meant to run in a hot loop.
export function describeScheduleStatus(rules: ScheduleRuleConfig[], at: Date, horizonMinutes = 7 * 24 * 60): ScheduleStatusPreview {
  const active = isScheduleActiveAt(rules, at)
  const start = new Date(at)
  start.setSeconds(0, 0)
  for (let minute = 1; minute <= horizonMinutes; minute += 1) {
    const candidate = new Date(start.getTime() + minute * 60000)
    if (isScheduleActiveAt(rules, candidate) !== active) {
      return { active, nextChangeAt: candidate }
    }
  }
  return { active, nextChangeAt: null }
}
