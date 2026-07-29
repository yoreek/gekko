import type {
  TimeSettingsRecord,
  TimeStatusResponse,
  TimezoneCatalogEntry,
} from '@/api'

export interface TimeSettingsSnapshot {
  status: TimeStatusResponse
  settings: TimeSettingsRecord
  timezones: TimezoneCatalogEntry[]
}

export interface TimeSettingsDraft {
  enabled: boolean
  ntpServer: string
  timezoneId: string
  syncIntervalSeconds: number
}

export function createTimeSettingsDraft(source: TimeSettingsSnapshot): TimeSettingsDraft {
  return {
    enabled: source.settings.enabled,
    ntpServer: source.settings.ntpServer,
    timezoneId: source.settings.timezoneId,
    syncIntervalSeconds: source.settings.syncIntervalSeconds,
  }
}

export function isTimeSettingsDirty(
  source: TimeSettingsSnapshot,
  draft: TimeSettingsDraft,
): boolean {
  return source.settings.enabled !== draft.enabled
    || source.settings.ntpServer !== draft.ntpServer
    || source.settings.timezoneId !== draft.timezoneId
    || source.settings.syncIntervalSeconds !== draft.syncIntervalSeconds
}

export function buildTimeSettingsPayload(draft: TimeSettingsDraft): TimeSettingsRecord {
  return {
    enabled: draft.enabled,
    ntpServer: draft.ntpServer,
    timezoneId: draft.timezoneId,
    syncIntervalSeconds: draft.syncIntervalSeconds,
  }
}
