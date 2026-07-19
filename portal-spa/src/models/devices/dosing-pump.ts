import type { BaseDeviceConfig, DeviceDependencyLink, DeviceRecord, DosingPumpOutputSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, encodeBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'
import { DOSING_PUMP_MAX_DOSES, minuteOfDayFromTime } from './dosing-pump-math.ts'

export type DosingScheduleMode = 'daily' | 'weekly'

export interface DosingPumpDoseDraft {
  time: string // "HH:mm"
  amountMl: number
}

export interface DosingPumpContainerDraft {
  capacityMl: number
  thresholdPercent: number
  blockAutoWhenEmpty: boolean
}

export interface DosingPumpScheduleDraft {
  mode: DosingScheduleMode
  everyDays: number
  daysOfWeek: number[]
  anchorDay: number
  doses: DosingPumpDoseDraft[]
}

export interface DosingPumpConfigDraft extends BaseDeviceConfig {
  dosingSpeedMlPerSec: number
  container: DosingPumpContainerDraft
  schedule: DosingPumpScheduleDraft
  // Dependency projections (not persisted config fields - they become `deps` links on save).
  pumpSwitchDeviceId: number
  levelSensorDeviceId: number
  levelSensorInvert: boolean
}

export interface DosingPumpCreateDraft extends DeviceCreateDraftBase, DosingPumpConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

function normalizeSpeed(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 && numeric <= 65.535 ? Math.round(numeric * 1000) / 1000 : fallback
}

function normalizeCapacityMl(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 1 && numeric <= 65535 ? Math.round(numeric) : fallback
}

function normalizeThresholdPercent(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 0 && numeric <= 100 ? Math.round(numeric) : fallback
}

function normalizeEveryDays(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 1 && numeric <= 30 ? Math.round(numeric) : fallback
}

function normalizeAnchorDay(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 0 && numeric <= 65535 ? Math.round(numeric) : fallback
}

function normalizeDaysOfWeek(value: unknown, fallback: number[]): number[] {
  if (!Array.isArray(value)) {
    return [...fallback]
  }
  const days = value
    .map(entry => Number(entry))
    .filter(day => Number.isInteger(day) && day >= 0 && day <= 6)
  return [...new Set(days)].sort((a, b) => a - b)
}

function normalizeDoses(value: unknown, fallback: DosingPumpDoseDraft[]): DosingPumpDoseDraft[] {
  if (!Array.isArray(value)) {
    return fallback.map(dose => ({ ...dose }))
  }
  return value
    .filter((entry): entry is Record<string, unknown> => isRecord(entry))
    .map(entry => ({
      time: typeof entry.time === 'string' ? entry.time : '',
      amountMl: Number(entry.amountMl),
    }))
    .filter(dose => minuteOfDayFromTime(dose.time) >= 0 && Number.isFinite(dose.amountMl) && dose.amountMl > 0)
    .slice(0, DOSING_PUMP_MAX_DOSES)
}

function normalizeContainer(value: unknown, defaults: DosingPumpContainerDraft): DosingPumpContainerDraft {
  if (!isRecord(value)) {
    return { ...defaults }
  }
  return {
    capacityMl: normalizeCapacityMl(value.capacityMl, defaults.capacityMl),
    thresholdPercent: normalizeThresholdPercent(value.thresholdPercent, defaults.thresholdPercent),
    blockAutoWhenEmpty: typeof value.blockAutoWhenEmpty === 'boolean' ? value.blockAutoWhenEmpty : defaults.blockAutoWhenEmpty,
  }
}

function normalizeSchedule(value: unknown, defaults: DosingPumpScheduleDraft): DosingPumpScheduleDraft {
  if (!isRecord(value)) {
    return { ...defaults, daysOfWeek: [...defaults.daysOfWeek], doses: defaults.doses.map(dose => ({ ...dose })) }
  }
  return {
    mode: value.mode === 'weekly' ? 'weekly' : value.mode === 'daily' ? 'daily' : defaults.mode,
    everyDays: normalizeEveryDays(value.everyDays, defaults.everyDays),
    daysOfWeek: normalizeDaysOfWeek(value.daysOfWeek, defaults.daysOfWeek),
    anchorDay: normalizeAnchorDay(value.anchorDay, defaults.anchorDay),
    doses: normalizeDoses(value.doses, defaults.doses),
  }
}

// Doses must be sorted by unique time before hitting the firmware validator - normalize the
// order here so the editor can leave rows wherever the user typed them.
export function sortDosesByTime(doses: DosingPumpDoseDraft[]): DosingPumpDoseDraft[] {
  return [...doses].sort((a, b) => minuteOfDayFromTime(a.time) - minuteOfDayFromTime(b.time))
}

export class DosingPumpDevice extends BaseDevice<DosingPumpConfigDraft, DosingPumpCreateDraft, DosingPumpOutputSnapshot> {
  static readonly TYPE_ID = 19 as const
  static readonly TYPE_NAME = 'dosing_pump' as const

  readonly typeName = DosingPumpDevice.TYPE_NAME
  readonly typeId = DosingPumpDevice.TYPE_ID

  static defaultConfig(): DosingPumpConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      dosingSpeedMlPerSec: 1,
      container: { capacityMl: 1000, thresholdPercent: 10, blockAutoWhenEmpty: true },
      schedule: { mode: 'daily', everyDays: 1, daysOfWeek: [0, 1, 2, 3, 4, 5, 6], anchorDay: 0, doses: [] },
      pumpSwitchDeviceId: 0,
      levelSensorDeviceId: 0,
      levelSensorInvert: false,
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): DosingPumpConfigDraft {
    const defaults = DosingPumpDevice.defaultConfig()
    const switchDeviceId = deps?.find(dep => dep.role === 'switch')?.deviceId ?? 0
    const sensorLink = deps?.find(dep => dep.role === 'condition')
    if (!isRecord(value)) {
      return {
        ...defaults,
        deps: Array.isArray(deps) ? deps : defaults.deps,
        pumpSwitchDeviceId: switchDeviceId,
        levelSensorDeviceId: sensorLink?.deviceId ?? 0,
        levelSensorInvert: sensorLink?.invert === true,
      }
    }
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      dosingSpeedMlPerSec: normalizeSpeed(value.dosingSpeedMlPerSec, defaults.dosingSpeedMlPerSec),
      container: normalizeContainer(value.container, defaults.container),
      schedule: normalizeSchedule(value.schedule, defaults.schedule),
      pumpSwitchDeviceId: normalizeDeviceId(value.pumpSwitchDeviceId ?? switchDeviceId),
      levelSensorDeviceId: normalizeDeviceId(value.levelSensorDeviceId ?? sensorLink?.deviceId),
      levelSensorInvert:
        typeof value.levelSensorInvert === 'boolean' ? value.levelSensorInvert : sensorLink?.invert === true,
    }
  }

  static dependencyLinks(config: DosingPumpConfigDraft): DeviceDependencyLink[] {
    const links: DeviceDependencyLink[] = [{ role: 'switch', deviceId: config.pumpSwitchDeviceId }]
    if (config.levelSensorDeviceId > 0) {
      links.push({ role: 'condition', deviceId: config.levelSensorDeviceId, invert: config.levelSensorInvert })
    }
    return links
  }

  createDefaultConfig(): DosingPumpConfigDraft {
    return DosingPumpDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): DosingPumpCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): DosingPumpCreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): DosingPumpConfigDraft {
    return DosingPumpDevice.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): DosingPumpOutputSnapshot {
    return ((record.runtime as { output?: DosingPumpOutputSnapshot }).output ?? {}) as DosingPumpOutputSnapshot
  }

  // The dependency projections (pumpSwitchDeviceId/levelSensor*) never travel inside the config
  // payload - they become deps links via createCreateDeps(). Doses are sorted on the way out so
  // the firmware's "sorted by unique time" validation is satisfied regardless of editor order.
  protected override encodeConfig(config: DosingPumpConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      dosingSpeedMlPerSec: config.dosingSpeedMlPerSec,
      container: { ...config.container },
      schedule: {
        mode: config.schedule.mode,
        everyDays: config.schedule.everyDays,
        daysOfWeek: [...config.schedule.daysOfWeek],
        anchorDay: config.schedule.anchorDay,
        doses: sortDosesByTime(config.schedule.doses).map(dose => ({ time: dose.time, amountMl: dose.amountMl })),
      },
    }
  }

  protected override createCreateDeps(config: DosingPumpConfigDraft): DeviceDependencyLink[] {
    return DosingPumpDevice.dependencyLinks(config)
  }
}
