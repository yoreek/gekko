import type { DoseJournalResponse } from './contracts'
import { detectTransportMode } from './transport'
import { requestJson } from './http'

export function fetchDoseJournal(deviceId: number, periodDays: number): Promise<DoseJournalResponse> {
  if (import.meta.env.DEV && detectTransportMode() === 'mock') {
    return import('@/mock/handlers').then(m => m.mockFetchDoseJournal(deviceId, periodDays))
  }
  const params = new URLSearchParams()
  if (deviceId > 0) {
    params.set('deviceId', String(deviceId))
  }
  params.set('periodDays', String(periodDays))
  return requestJson<DoseJournalResponse>(`/api/dosejournal?${params.toString()}`)
}
