import type { ScheduledAnalogOutputPointDraft } from './composable-analog-output'

export const analogScheduleMinutesPerDay = 1440
export const analogScheduleMaximumPointCount = 10
export const analogScheduleInsertionOffsetMinutes = 30
export const analogScheduleDefaultTimeGridStepMinutes = 15
export const analogScheduleDefaultLevelGridStepPercent = 5
export const analogScheduleMinimumTimeGridStepMinutes = 5
export const analogScheduleMaximumTimeGridStepMinutes = 240

export interface AnalogScheduleChartChannel {
  id: number
  name: string
  points: ScheduledAnalogOutputPointDraft[]
  editable?: boolean
}

function clampInteger(value: number, minimum: number, maximum: number): number {
  if (!Number.isFinite(value)) {
    return minimum
  }
  return Math.min(maximum, Math.max(minimum, Math.round(value)))
}

export function normalizeAnalogScheduleGridStep(
  value: number,
  minimum: number,
  maximum: number,
  fallback: number,
): number {
  if (!Number.isFinite(value)) {
    return fallback
  }
  return clampInteger(value, minimum, maximum)
}

export function formatAnalogScheduleTime(minuteOfDay: number): string {
  const minute = clampInteger(minuteOfDay, 0, analogScheduleMinutesPerDay - 1)
  return `${String(Math.floor(minute / 60)).padStart(2, '0')}:${String(minute % 60).padStart(2, '0')}`
}

export function parseAnalogScheduleTime(value: string): number {
  const match = /^(\d{2}):(\d{2})$/.exec(value)
  if (match === null) {
    return 0
  }
  const hours = Number(match[1])
  const minutes = Number(match[2])
  if (hours > 23 || minutes > 59) {
    return 0
  }
  return hours * 60 + minutes
}

export function activeAnalogSchedulePoints(
  points: ScheduledAnalogOutputPointDraft[],
): ScheduledAnalogOutputPointDraft[] {
  return points
    .filter(point => !point.deleted)
    .slice()
    .sort((left, right) => left.minuteOfDay - right.minuteOfDay)
}

export function sampleAnalogSchedule(
  points: ScheduledAnalogOutputPointDraft[],
  minuteOfDay: number,
): number {
  const active = activeAnalogSchedulePoints(points)
  if (active.length === 0) {
    return 0
  }
  if (active.length === 1) {
    return active[0].state
  }

  const minute = clampInteger(minuteOfDay, 0, analogScheduleMinutesPerDay - 1)
  let previous = active[active.length - 1]
  let next = active[0]
  for (const point of active) {
    if (point.minuteOfDay <= minute) {
      previous = point
    }
    if (point.minuteOfDay >= minute) {
      next = point
      break
    }
  }

  const previousTime = previous.minuteOfDay > minute
    ? previous.minuteOfDay - analogScheduleMinutesPerDay
    : previous.minuteOfDay
  const nextTime = next.minuteOfDay < minute
    ? next.minuteOfDay + analogScheduleMinutesPerDay
    : next.minuteOfDay
  if (nextTime === previousTime) {
    return previous.state
  }
  return Math.round(
    previous.state
      + (next.state - previous.state) * ((minute - previousTime) / (nextTime - previousTime)),
  )
}

function nearestAvailableMinute(
  requestedMinute: number,
  occupiedMinutes: Set<number>,
  stepMinutes: number,
): number {
  const maximum = analogScheduleMinutesPerDay - 1
  const snapped = clampInteger(
    Math.round(requestedMinute / stepMinutes) * stepMinutes,
    0,
    maximum,
  )
  if (!occupiedMinutes.has(snapped)) {
    return snapped
  }

  for (let distance = stepMinutes; distance < analogScheduleMinutesPerDay; distance += stepMinutes) {
    const later = snapped + distance
    if (later <= maximum && !occupiedMinutes.has(later)) {
      return later
    }
    const earlier = snapped - distance
    if (earlier >= 0 && !occupiedMinutes.has(earlier)) {
      return earlier
    }
  }
  return snapped
}

export function moveAnalogSchedulePoint(
  points: ScheduledAnalogOutputPointDraft[],
  pointIndex: number,
  minuteOfDay: number,
  state: number,
  stepMinutes = analogScheduleDefaultTimeGridStepMinutes,
  stepPercent = analogScheduleDefaultLevelGridStepPercent,
): ScheduledAnalogOutputPointDraft[] {
  if (
    pointIndex < 0
    || pointIndex >= points.length
    || points[pointIndex].deleted
    || points.filter(point => !point.deleted).length <= 1
  ) {
    return points
  }

  const occupiedMinutes = new Set(
    points
      .filter((point, index) => index !== pointIndex && !point.deleted)
      .map(point => point.minuteOfDay),
  )
  const minute = nearestAvailableMinute(
    minuteOfDay,
    occupiedMinutes,
    Math.max(1, Math.round(stepMinutes)),
  )
  const normalizedPercentStep = Math.max(1, Math.round(stepPercent))
  return points.map((point, index) => index === pointIndex
    ? {
        ...point,
        minuteOfDay: minute,
        state: clampInteger(
          Math.round(state / normalizedPercentStep) * normalizedPercentStep,
          0,
          100,
        ),
      }
    : point)
}

export function flatAnalogSchedulePoints(state: number): ScheduledAnalogOutputPointDraft[] {
  return [{
    deleted: false,
    minuteOfDay: 0,
    state: clampInteger(state, 0, 100),
  }]
}

function availablePointIndex(points: ScheduledAnalogOutputPointDraft[]): number {
  const deletedIndex = points.findIndex(point => point.deleted)
  if (deletedIndex >= 0) {
    return deletedIndex
  }
  return points.length < analogScheduleMaximumPointCount ? points.length : -1
}

function replaceOrAppendPoint(
  points: ScheduledAnalogOutputPointDraft[],
  index: number,
  point: ScheduledAnalogOutputPointDraft,
): ScheduledAnalogOutputPointDraft[] {
  if (index < 0) {
    return points
  }
  if (index === points.length) {
    return [...points, point]
  }
  return points.map((current, currentIndex) => currentIndex === index ? point : current)
}

export function addAnalogSchedulePoint(
  points: ScheduledAnalogOutputPointDraft[],
  minuteOfDay: number,
  state: number,
  stepMinutes = analogScheduleDefaultTimeGridStepMinutes,
  stepPercent = analogScheduleDefaultLevelGridStepPercent,
): ScheduledAnalogOutputPointDraft[] {
  const index = availablePointIndex(points)
  if (index < 0) {
    return points
  }
  const occupiedMinutes = new Set(
    points.filter(point => !point.deleted).map(point => point.minuteOfDay),
  )
  const normalizedPercentStep = Math.max(1, Math.round(stepPercent))
  return replaceOrAppendPoint(points, index, {
    deleted: false,
    minuteOfDay: nearestAvailableMinute(
      minuteOfDay,
      occupiedMinutes,
      Math.max(1, Math.round(stepMinutes)),
    ),
    state: clampInteger(
      Math.round(state / normalizedPercentStep) * normalizedPercentStep,
      0,
      100,
    ),
  })
}

export function deleteAnalogSchedulePoint(
  points: ScheduledAnalogOutputPointDraft[],
  pointIndex: number,
): ScheduledAnalogOutputPointDraft[] {
  if (pointIndex < 0 || pointIndex >= points.length || points[pointIndex].deleted) {
    return points
  }
  return points.map((point, index) => index === pointIndex ? { ...point, deleted: true } : point)
}

function availableMinuteInDirection(
  sourceMinute: number,
  occupiedMinutes: Set<number>,
  direction: -1 | 1,
  offsetMinutes: number,
): number {
  const offset = Math.max(1, Math.round(offsetMinutes))
  for (let distance = offset; distance < analogScheduleMinutesPerDay; distance += offset) {
    const candidate = (
      sourceMinute + direction * distance + analogScheduleMinutesPerDay
    ) % analogScheduleMinutesPerDay
    if (!occupiedMinutes.has(candidate)) {
      return candidate
    }
  }
  return sourceMinute
}

export function insertAnalogSchedulePoint(
  points: ScheduledAnalogOutputPointDraft[],
  sourceIndex: number,
  direction: -1 | 1,
  offsetMinutes = analogScheduleInsertionOffsetMinutes,
): ScheduledAnalogOutputPointDraft[] {
  if (
    sourceIndex < 0
    || sourceIndex >= points.length
    || points[sourceIndex].deleted
  ) {
    return points
  }
  const targetIndex = availablePointIndex(points)
  if (targetIndex < 0) {
    return points
  }
  const source = points[sourceIndex]
  const occupiedMinutes = new Set(
    points.filter(point => !point.deleted).map(point => point.minuteOfDay),
  )
  return replaceOrAppendPoint(points, targetIndex, {
    deleted: false,
    minuteOfDay: availableMinuteInDirection(
      source.minuteOfDay,
      occupiedMinutes,
      direction,
      offsetMinutes,
    ),
    state: source.state,
  })
}
