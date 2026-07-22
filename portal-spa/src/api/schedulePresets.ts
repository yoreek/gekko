import type { SchedulePresetPoint, SchedulePresetsResponse } from './contracts'
import { detectTransportMode } from './transport'
import { requestEmpty, requestJson } from './http'

function useMock(): boolean {
  return import.meta.env.DEV && detectTransportMode() === 'mock'
}

export function fetchSchedulePresets(deviceId: number): Promise<SchedulePresetsResponse> {
  if (useMock()) {
    return import('@/mock/handlers').then(m => m.mockFetchSchedulePresets(deviceId))
  }
  return requestJson<SchedulePresetsResponse>(`/api/schedulepresets/${deviceId}`)
}

export function saveSchedulePreset(deviceId: number, slot: number, name: string, points: SchedulePresetPoint[]): Promise<void> {
  if (useMock()) {
    return import('@/mock/handlers').then(m => m.mockSaveSchedulePreset(deviceId, slot, name, points))
  }
  return requestEmpty(`/api/schedulepresets/${deviceId}/${slot}`, {
    method: 'PUT',
    body: JSON.stringify({ name, points }),
  })
}

export function deleteSchedulePreset(deviceId: number, slot: number): Promise<void> {
  if (useMock()) {
    return import('@/mock/handlers').then(m => m.mockDeleteSchedulePreset(deviceId, slot))
  }
  return requestEmpty(`/api/schedulepresets/${deviceId}/${slot}`, { method: 'DELETE' })
}
