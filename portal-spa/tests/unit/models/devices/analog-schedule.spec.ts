import assert from 'node:assert/strict'
import test from 'node:test'

import {
  addAnalogSchedulePoint,
  deleteAnalogSchedulePoint,
  formatAnalogScheduleTime,
  insertAnalogSchedulePoint,
  moveAnalogSchedulePoint,
  normalizeAnalogScheduleGridStep,
  parseAnalogScheduleTime,
  sampleAnalogSchedule,
} from '../../../../src/models/devices/analog-schedule.ts'

test('analog schedule samples cyclic linear interpolation across midnight', () => {
  const points = [
    { deleted: false, minuteOfDay: 60, state: 100 },
    { deleted: false, minuteOfDay: 1380, state: 0 },
  ]

  assert.equal(sampleAnalogSchedule(points, 0), 50)
  assert.equal(sampleAnalogSchedule(points, 60), 100)
  assert.equal(sampleAnalogSchedule(points, 1380), 0)
})

test('analog schedule point movement uses the default grid and avoids duplicates', () => {
  const points = [
    { deleted: false, minuteOfDay: 60, state: 10 },
    { deleted: false, minuteOfDay: 120, state: 20 },
  ]

  const moved = moveAnalogSchedulePoint(points, 1, 61, 120)

  assert.deepEqual(moved, [
    { deleted: false, minuteOfDay: 60, state: 10 },
    { deleted: false, minuteOfDay: 75, state: 100 },
  ])
  assert.equal(points[1].minuteOfDay, 120)
})

test('analog schedule point movement accepts configured time and level grid steps', () => {
  const points = [{ deleted: false, minuteOfDay: 60, state: 10 }]

  assert.deepEqual(
    moveAnalogSchedulePoint(points, 0, 68, 52, 15, 5),
    [{ deleted: false, minuteOfDay: 75, state: 50 }],
  )
  assert.deepEqual(
    moveAnalogSchedulePoint(points, 0, 68, 52, 1, 1),
    [{ deleted: false, minuteOfDay: 68, state: 52 }],
  )
})

test('analog schedule grid steps are normalized to editor limits', () => {
  assert.equal(normalizeAnalogScheduleGridStep(Number.NaN, 5, 240, 15), 15)
  assert.equal(normalizeAnalogScheduleGridStep(2, 5, 240, 15), 5)
  assert.equal(normalizeAnalogScheduleGridStep(500, 5, 240, 15), 240)
  assert.equal(normalizeAnalogScheduleGridStep(14.6, 5, 240, 15), 15)
})

test('analog schedule time helpers use a strict 24-hour field shape', () => {
  assert.equal(formatAnalogScheduleTime(510), '08:30')
  assert.equal(parseAnalogScheduleTime('08:30'), 510)
  assert.equal(parseAnalogScheduleTime('24:00'), 0)
  assert.equal(parseAnalogScheduleTime('8:30'), 0)
})

test('analog schedule reuses deleted slots when adding and deleting points', () => {
  const points = [
    { deleted: false, minuteOfDay: 60, state: 10 },
    { deleted: true, minuteOfDay: 120, state: 20 },
  ]

  const added = addAnalogSchedulePoint(points, 61, 52, 15, 5)
  assert.deepEqual(added, [
    { deleted: false, minuteOfDay: 60, state: 10 },
    { deleted: false, minuteOfDay: 75, state: 50 },
  ])
  assert.deepEqual(deleteAnalogSchedulePoint(added, 0), [
    { deleted: true, minuteOfDay: 60, state: 10 },
    { deleted: false, minuteOfDay: 75, state: 50 },
  ])
})

test('analog schedule inserts before and after by 30 minutes without overlaps', () => {
  const points = [
    { deleted: false, minuteOfDay: 60, state: 40 },
    { deleted: false, minuteOfDay: 90, state: 50 },
  ]

  assert.deepEqual(insertAnalogSchedulePoint(points, 0, -1), [
    ...points,
    { deleted: false, minuteOfDay: 30, state: 40 },
  ])
  assert.deepEqual(insertAnalogSchedulePoint(points, 0, 1), [
    ...points,
    { deleted: false, minuteOfDay: 120, state: 40 },
  ])
})
