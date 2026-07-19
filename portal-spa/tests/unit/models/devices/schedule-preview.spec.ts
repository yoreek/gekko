import assert from 'node:assert/strict'
import test from 'node:test'

import {
  describeScheduleStatus,
  intervalDurationExceedsSlice,
  intervalSegmentStarts,
  isRuleActive,
  isScheduleActiveAt,
} from '../../../../src/models/devices/schedule-preview.ts'
import type { ScheduleRuleConfig } from '../../../../src/api/contracts.ts'

function makeRule(overrides: Partial<ScheduleRuleConfig> = {}): ScheduleRuleConfig {
  return {
    enabled: true,
    weekDays: [0, 1, 2, 3, 4, 5, 6],
    startMinuteOfDay: 8 * 60,
    endMinuteOfDay: 20 * 60,
    mode: 'alwaysOn',
    intervalsPerWindow: 1,
    durationMinutes: 1,
    ...overrides,
  }
}

test('isRuleActive respects the enabled flag, weekday mask, and time range', () => {
  const rule = makeRule({ weekDays: [1, 2, 3, 4, 5] })
  // Monday (1) at 12:00 - within range and weekday.
  assert.equal(isRuleActive(rule, 1, 12 * 60), true)
  // Sunday (0) - not in weekDays.
  assert.equal(isRuleActive(rule, 0, 12 * 60), false)
  // Monday but before the window opens.
  assert.equal(isRuleActive(rule, 1, 7 * 60), false)
  // Disabled rule is never active regardless of day/time.
  assert.equal(isRuleActive(makeRule({ enabled: false }), 1, 12 * 60), false)
})

test('isRuleActive handles a midnight-wrapping window', () => {
  const rule = makeRule({ startMinuteOfDay: 22 * 60, endMinuteOfDay: 6 * 60 })
  assert.equal(isRuleActive(rule, 3, 23 * 60), true)
  assert.equal(isRuleActive(rule, 3, 2 * 60), true)
  assert.equal(isRuleActive(rule, 3, 12 * 60), false)
})

test('isScheduleActiveAt ORs across multiple rules', () => {
  const weekdayRule = makeRule({ weekDays: [1, 2, 3, 4, 5], startMinuteOfDay: 9 * 60, endMinuteOfDay: 17 * 60 })
  const weekendRule = makeRule({ weekDays: [0, 6], startMinuteOfDay: 10 * 60, endMinuteOfDay: 12 * 60 })
  const rules = [weekdayRule, weekendRule]
  // Monday 2026-07-06 10:00 - matches the weekday rule.
  assert.equal(isScheduleActiveAt(rules, new Date(2026, 6, 6, 10, 0)), true)
  // Sunday 2026-07-05 11:00 - matches the weekend rule.
  assert.equal(isScheduleActiveAt(rules, new Date(2026, 6, 5, 11, 0)), true)
  // Sunday 2026-07-05 15:00 - matches neither.
  assert.equal(isScheduleActiveAt(rules, new Date(2026, 6, 5, 15, 0)), false)
})

test('describeScheduleStatus finds the next same-day transition', () => {
  const rule = makeRule({ startMinuteOfDay: 8 * 60, endMinuteOfDay: 20 * 60 })
  const at = new Date(2026, 6, 6, 10, 0) // Monday 10:00, inside the window
  const result = describeScheduleStatus([rule], at)
  assert.equal(result.active, true)
  assert.ok(result.nextChangeAt)
  assert.equal(result.nextChangeAt?.getHours(), 20)
  assert.equal(result.nextChangeAt?.getMinutes(), 0)
})

test('describeScheduleStatus reports no change within the horizon when nothing ever flips', () => {
  const alwaysRule = makeRule({ startMinuteOfDay: 0, endMinuteOfDay: 0 }) // start===end -> whole day
  const result = describeScheduleStatus([alwaysRule], new Date(2026, 6, 6, 10, 0), 60)
  assert.equal(result.active, true)
  assert.equal(result.nextChangeAt, null)
})

test('intervalSegmentStarts is empty for AlwaysOn rules', () => {
  assert.deepEqual(intervalSegmentStarts(makeRule({ mode: 'alwaysOn' })), [])
})

test('intervalSegmentStarts is empty when intervalsPerWindow is invalid', () => {
  assert.deepEqual(intervalSegmentStarts(makeRule({ mode: 'interval', intervalsPerWindow: 0 })), [])
  assert.deepEqual(intervalSegmentStarts(makeRule({ mode: 'interval', intervalsPerWindow: -1 })), [])
})

test('intervalSegmentStarts is empty when the window is shorter than intervalsPerWindow', () => {
  // 3-minute window, 5 intervals requested -> intervalMinutes floors to 0.
  const rule = makeRule({ mode: 'interval', startMinuteOfDay: 0, endMinuteOfDay: 3, intervalsPerWindow: 5, durationMinutes: 1 })
  assert.deepEqual(intervalSegmentStarts(rule), [])
})

test('intervalSegmentStarts splits an evenly-dividing window into equal slices', () => {
  // 08:00-20:00 (720 minutes), 5 intervals -> 144-minute slices, 12-minute on-duration.
  const rule = makeRule({ mode: 'interval', startMinuteOfDay: 8 * 60, endMinuteOfDay: 20 * 60, intervalsPerWindow: 5, durationMinutes: 12 })
  const segments = intervalSegmentStarts(rule)
  assert.equal(segments.length, 5)
  assert.deepEqual(
    segments.map(s => s.sliceStartMinuteOfDay),
    [8 * 60, 8 * 60 + 144, 8 * 60 + 288, 8 * 60 + 432, 8 * 60 + 576],
  )
  assert.deepEqual(
    segments.map(s => s.onEndMinuteOfDay),
    segments.map(s => s.sliceStartMinuteOfDay + 12),
  )
  assert.ok(segments.every(s => s.sliceMinutes === 144))
})

test('intervalSegmentStarts wraps slice starts past midnight', () => {
  // 23:00-01:00 wraps (120 minutes total), 3 intervals -> 40-minute slices.
  const rule = makeRule({ mode: 'interval', startMinuteOfDay: 23 * 60, endMinuteOfDay: 1 * 60, intervalsPerWindow: 3, durationMinutes: 10 })
  const segments = intervalSegmentStarts(rule)
  assert.deepEqual(
    segments.map(s => s.sliceStartMinuteOfDay),
    [23 * 60, 23 * 60 + 40 - 1440, 23 * 60 + 80 - 1440].map(m => ((m % 1440) + 1440) % 1440),
  )
})

test('intervalSegmentStarts treats a start===end window as the whole day', () => {
  const rule = makeRule({ mode: 'interval', startMinuteOfDay: 6 * 60, endMinuteOfDay: 6 * 60, intervalsPerWindow: 4, durationMinutes: 5 })
  const segments = intervalSegmentStarts(rule)
  assert.equal(segments.length, 4)
  assert.ok(segments.every(s => s.sliceMinutes === 360))
})

test('intervalSegmentStarts appends a trailing partial slice when the window does not divide evenly', () => {
  // 100-minute window, 3 intervals -> 33-minute slices with 1 leftover minute.
  const rule = makeRule({ mode: 'interval', startMinuteOfDay: 0, endMinuteOfDay: 100, intervalsPerWindow: 3, durationMinutes: 5 })
  const segments = intervalSegmentStarts(rule)
  assert.equal(segments.length, 4)
  assert.deepEqual(segments.slice(0, 3).map(s => s.sliceMinutes), [33, 33, 33])
  const trailing = segments[3]
  assert.equal(trailing.sliceMinutes, 1)
  assert.equal(trailing.sliceStartMinuteOfDay, 99)
  // durationMinutes (5) exceeds the 1-minute trailing slice - clipped to the slice length.
  assert.equal(trailing.onEndMinuteOfDay, 100)
})

test('intervalSegmentStarts clips onEndMinuteOfDay when durationMinutes exceeds the slice length', () => {
  // 60-minute window, 4 intervals -> 15-minute slices; duration of 30 exceeds every slice.
  const rule = makeRule({ mode: 'interval', startMinuteOfDay: 0, endMinuteOfDay: 60, intervalsPerWindow: 4, durationMinutes: 30 })
  const segments = intervalSegmentStarts(rule)
  assert.equal(segments.length, 4)
  for (const segment of segments) {
    assert.equal(segment.onEndMinuteOfDay, segment.sliceStartMinuteOfDay + segment.sliceMinutes)
    assert.ok(segment.sliceMinutes < rule.durationMinutes)
  }
})

test('intervalDurationExceedsSlice flags a genuinely broken (always-on) configuration', () => {
  // 60-minute window, 4 intervals -> 15-minute slices; duration of 30 exceeds every regular slice.
  const rule = makeRule({ mode: 'interval', startMinuteOfDay: 0, endMinuteOfDay: 60, intervalsPerWindow: 4, durationMinutes: 30 })
  assert.equal(intervalDurationExceedsSlice(rule), true)
})

test('intervalDurationExceedsSlice does not flag a correctly duty-cycling rule just because the trailing remainder is short', () => {
  // 08:00-20:00 (720 minutes), 7 intervals -> 102-minute regular slices with a 6-minute remainder.
  // durationMinutes=7 is far shorter than the regular 102-minute slice (correct duty-cycling) even
  // though it happens to exceed the 6-minute trailing remainder - must not warn.
  const rule = makeRule({ mode: 'interval', startMinuteOfDay: 8 * 60, endMinuteOfDay: 20 * 60, intervalsPerWindow: 7, durationMinutes: 7 })
  const segments = intervalSegmentStarts(rule)
  assert.equal(segments.length, 8) // 7 regular + 1 trailing remainder
  assert.equal(segments[7].sliceMinutes, 6)
  assert.equal(intervalDurationExceedsSlice(rule), false)
})

test('intervalDurationExceedsSlice is false for AlwaysOn rules and invalid interval config', () => {
  assert.equal(intervalDurationExceedsSlice(makeRule({ mode: 'alwaysOn' })), false)
  assert.equal(intervalDurationExceedsSlice(makeRule({ mode: 'interval', intervalsPerWindow: 0 })), false)
})
