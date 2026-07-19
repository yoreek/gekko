import { defineStore } from 'pinia'

import type { TimeSettingsRecord, TimeStatusResponse } from '@/api'

export const useTimeStore = defineStore('time', {
  state: () => ({
    enabled: true,
    synced: false,
    waitingForStation: false,
    ntpServer: 'pool.ntp.org',
    timezoneId: 'Etc/GMT',
    syncIntervalSeconds: 3600,
    source: 'ntp' as 'ntp' | 'manual' | 'rtc',
    currentEpochUtc: 0,
    lastSyncEpochUtc: 0,
    localTimeIso8601: '',
    utcOffsetMinutes: 0,
    timezoneAbbrev: '',
  }),
  actions: {
    replaceFromStatus(payload: TimeStatusResponse): void {
      this.enabled = payload.enabled
      this.synced = payload.synced
      this.waitingForStation = payload.waitingForStation
      this.ntpServer = payload.ntpServer
      this.timezoneId = payload.timezoneId
      this.syncIntervalSeconds = payload.syncIntervalSeconds
      this.source = payload.source
      this.currentEpochUtc = payload.currentEpochUtc ?? this.currentEpochUtc
      this.lastSyncEpochUtc = payload.lastSyncEpochUtc ?? this.lastSyncEpochUtc
      this.localTimeIso8601 = payload.localTimeIso8601 ?? this.localTimeIso8601
      this.utcOffsetMinutes = payload.utcOffsetMinutes ?? this.utcOffsetMinutes
      this.timezoneAbbrev = payload.timezoneAbbrev ?? this.timezoneAbbrev
    },
    replaceFromSettings(payload: TimeSettingsRecord): void {
      this.enabled = payload.enabled
      this.ntpServer = payload.ntpServer
      this.timezoneId = payload.timezoneId
      this.syncIntervalSeconds = payload.syncIntervalSeconds
    },
  },
})
