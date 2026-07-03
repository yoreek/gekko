import { defineStore } from 'pinia'

import type { DeviceRecord } from '@/api/contracts'
import { extractReadings, type HistorySeriesKind } from '@/models/devices/history'

export interface HistorySample {
  t: number
  v: number | null
}

interface HistorySeriesMeta {
  kind: HistorySeriesKind
  labelKey: string
  unitSymbol?: string
}

export interface HistorySeries extends HistorySeriesMeta {
  key: string
  samples: HistorySample[]
}

const kMaxSamplesPerSeries = 2000
const kRetentionMs = 24 * 60 * 60 * 1000

function seriesId(deviceId: number, seriesKey: string): string {
  return `${deviceId}:${seriesKey}`
}

export const useDeviceHistoryStore = defineStore('deviceHistory', {
  state: () => ({
    buffers: {} as Record<string, HistorySample[]>,
    meta: {} as Record<string, HistorySeriesMeta>,
  }),
  actions: {
    recordSample(device: DeviceRecord, at: number = Date.now()): void {
      const readings = extractReadings(device)
      for (const reading of readings) {
        if (reading.value === null) {
          continue
        }
        const id = seriesId(device.record.id, reading.key)
        this.meta[id] = {
          kind: reading.kind,
          labelKey: reading.labelKey,
          unitSymbol: reading.unitSymbol,
        }
        const samples = this.buffers[id] ?? []
        const last = samples[samples.length - 1]
        if (last && last.v === reading.value) {
          continue
        }
        samples.push({ t: at, v: reading.value })

        const cutoff = at - kRetentionMs
        let start = 0
        while (start < samples.length && samples[start].t < cutoff) {
          start += 1
        }
        const trimmed = start > 0 ? samples.slice(start) : samples
        this.buffers[id] = trimmed.length > kMaxSamplesPerSeries
          ? trimmed.slice(trimmed.length - kMaxSamplesPerSeries)
          : trimmed
      }
    },
    clear(): void {
      this.buffers = {}
      this.meta = {}
    },
  },
  getters: {
    seriesFor: state => (deviceId: number): HistorySeries[] => {
      return Object.keys(state.meta)
        .filter(id => id.startsWith(`${deviceId}:`))
        .map(id => ({
          key: id.slice(String(deviceId).length + 1),
          ...state.meta[id],
          samples: state.buffers[id] ?? [],
        }))
    },
  },
})
