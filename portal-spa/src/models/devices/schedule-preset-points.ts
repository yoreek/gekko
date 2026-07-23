import type { SchedulePresetPoint } from '@/api/contracts'
import type { ScheduledAnalogOutputPointDraft } from './composable-analog-output'

// Preset points (as carried by the REST API, state = percentage) -> editor draft points.
export function presetPointsToDraft(points: SchedulePresetPoint[]): ScheduledAnalogOutputPointDraft[] {
  return points.map(point => ({ deleted: false, minuteOfDay: point.minuteOfDay, state: point.state }))
}

// Editor draft points -> preset points to persist (drop deleted/empty slots).
export function draftPointsToPreset(points: ScheduledAnalogOutputPointDraft[]): SchedulePresetPoint[] {
  return points.filter(point => !point.deleted).map(point => ({ minuteOfDay: point.minuteOfDay, state: point.state }))
}
