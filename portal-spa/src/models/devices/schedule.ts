import type { BaseDeviceConfig, ScheduleOutputSnapshot, ScheduleRuleConfig, ScheduleRuleMode } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceRecord } from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'

export const kMaxScheduleRules = 4

export interface ScheduleConfigDraft extends BaseDeviceConfig {
  rules: ScheduleRuleConfig[]
}

export interface ScheduleCreateDraft extends DeviceCreateDraftBase, ScheduleConfigDraft {}

const ruleModeOptions: ScheduleRuleMode[] = ['alwaysOn', 'interval']

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeWeekDays(value: unknown): number[] {
  if (!Array.isArray(value)) {
    return [0, 1, 2, 3, 4, 5, 6]
  }
  const days = value
    .map(entry => Number(entry))
    .filter(entry => Number.isInteger(entry) && entry >= 0 && entry <= 6)
  return Array.from(new Set(days)).sort()
}

function normalizeMinuteOfDay(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 0 && numeric <= 1439 ? Math.round(numeric) : fallback
}

function normalizeMode(value: unknown): ScheduleRuleMode {
  return ruleModeOptions.includes(value as ScheduleRuleMode) ? (value as ScheduleRuleMode) : 'alwaysOn'
}

function normalizePositiveInt(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 0 ? Math.round(numeric) : fallback
}

export function defaultScheduleRule(): ScheduleRuleConfig {
  return {
    enabled: true,
    weekDays: [0, 1, 2, 3, 4, 5, 6],
    startMinuteOfDay: 8 * 60,
    endMinuteOfDay: 20 * 60,
    mode: 'alwaysOn',
    intervalsPerWindow: 1,
    durationMinutes: 1,
  }
}

export function normalizeScheduleRule(value: unknown): ScheduleRuleConfig {
  const defaults = defaultScheduleRule()
  if (!isRecord(value)) {
    return defaults
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : defaults.enabled,
    weekDays: normalizeWeekDays(value.weekDays),
    startMinuteOfDay: normalizeMinuteOfDay(value.startMinuteOfDay, defaults.startMinuteOfDay),
    endMinuteOfDay: normalizeMinuteOfDay(value.endMinuteOfDay, defaults.endMinuteOfDay),
    mode: normalizeMode(value.mode),
    intervalsPerWindow: Math.max(1, normalizePositiveInt(value.intervalsPerWindow, defaults.intervalsPerWindow)),
    durationMinutes: normalizePositiveInt(value.durationMinutes, defaults.durationMinutes),
  }
}

export class ScheduleDevice extends BaseDevice<ScheduleConfigDraft, ScheduleCreateDraft, ScheduleOutputSnapshot> {
  static readonly TYPE_ID = 15 as const
  static readonly TYPE_NAME = 'schedule' as const

  readonly typeName = ScheduleDevice.TYPE_NAME
  readonly typeId = ScheduleDevice.TYPE_ID
  // Schedule provides DeviceRole::Schedule, plus DeviceRole::Condition so AutoSwitchDevice can
  // attach it as an AND-condition source.
  readonly dependencyRoles: DeviceRole[] = ['schedule', 'condition']

  static defaultConfig(): ScheduleConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      rules: [],
    }
  }

  static normalizeConfig(value: unknown): ScheduleConfigDraft {
    const defaults = ScheduleDevice.defaultConfig()
    if (!isRecord(value)) {
      return defaults
    }
    const rawRules = Array.isArray(value.rules) ? value.rules : []
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      rules: rawRules.slice(0, kMaxScheduleRules).map(normalizeScheduleRule),
    }
  }

  createDefaultConfig(): ScheduleConfigDraft {
    return ScheduleDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): ScheduleCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): ScheduleCreateDraft {
    return {
      ...this.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown): ScheduleConfigDraft {
    return ScheduleDevice.normalizeConfig(value)
  }

  normalizeOutput(_record: DeviceRecord): ScheduleOutputSnapshot {
    return {}
  }
}
