import type { DoseJournalResponse } from './contracts'
import { detectTransportMode } from './transport'
import { requestJson } from './http'
import { mockFetchDoseJournal } from '@/mock/handlers'

export function fetchDoseJournal(deviceId: number, periodDays: number): Promise<DoseJournalResponse> {
  if (detectTransportMode() === 'mock') {
    return Promise.resolve(mockFetchDoseJournal(deviceId, periodDays))
  }
  const params = new URLSearchParams()
  if (deviceId > 0) {
    params.set('deviceId', String(deviceId))
  }
  params.set('periodDays', String(periodDays))
  return requestJson<DoseJournalResponse>(`/api/dosejournal?${params.toString()}`)
}
