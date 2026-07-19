import type {
  AnalogOutputComposerOutputSnapshot,
  BaseDeviceConfig,
  DeviceCommandRequest,
  DeviceDependencyLink,
  DeviceRecord,
  FadeAnalogOutputOutputSnapshot,
  ScheduledAnalogOutputOutputSnapshot,
} from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'
import type { DeviceCreateDraftBase } from './base.ts'
import { BaseDevice, defaultBaseDeviceConfig, encodeBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'

export interface AnalogOutputDependencyConfig extends BaseDeviceConfig {
  targetDeviceId: number
}

export interface FadeAnalogOutputConfigDraft extends AnalogOutputDependencyConfig {
  maxStep: number
  stepIntervalMs: number
}

export interface ScheduledAnalogOutputPointDraft {
  deleted: boolean
  minuteOfDay: number
  state: number
}

export interface ScheduledAnalogOutputConfigDraft extends AnalogOutputDependencyConfig {
  points: ScheduledAnalogOutputPointDraft[]
}

export interface AnalogOutputComposerConfigDraft extends BaseDeviceConfig {
  targetDeviceIds: number[]
  pendingCommands?: DeviceCommandRequest[]
}

type FadeCreateDraft = DeviceCreateDraftBase & FadeAnalogOutputConfigDraft
type ScheduledCreateDraft = DeviceCreateDraftBase & ScheduledAnalogOutputConfigDraft
type ComposerCreateDraft = DeviceCreateDraftBase & AnalogOutputComposerConfigDraft

const isRecord = (value: unknown): value is Record<string, unknown> =>
  typeof value === 'object' && value !== null && !Array.isArray(value)
const percent = (value: unknown, fallback = 0): number => {
  const number = Number(value)
  return Number.isFinite(number) ? Math.min(100, Math.max(0, Math.round(number))) : fallback
}
const deviceId = (value: unknown): number => {
  const number = Number(value)
  return Number.isFinite(number) && number > 0 ? Math.round(number) : 0
}
const depsFrom = (raw: Record<string, unknown>, deps?: DeviceDependencyLink[]): DeviceDependencyLink[] =>
  Array.isArray(raw.deps) ? raw.deps as DeviceDependencyLink[] : deps ?? []
const firstTarget = (deps: DeviceDependencyLink[]): number =>
  deps.find(link => link.role === 'analog_output')?.deviceId ?? 0
const targetDeps = (ids: number[]): DeviceDependencyLink[] =>
  ids.filter(id => id > 0).map(id => ({ role: 'analog_output', deviceId: id }))

export class FadeAnalogOutputDevice extends BaseDevice<FadeAnalogOutputConfigDraft, FadeCreateDraft, FadeAnalogOutputOutputSnapshot> {
  static readonly TYPE_ID = 21
  static readonly TYPE_NAME = 'fade_analog_output'
  readonly typeId = FadeAnalogOutputDevice.TYPE_ID
  readonly typeName = FadeAnalogOutputDevice.TYPE_NAME
  readonly dependencyRoles: DeviceRole[] = ['analog_output']

  createDefaultConfig(): FadeAnalogOutputConfigDraft {
    return { ...defaultBaseDeviceConfig(), targetDeviceId: 0, maxStep: 1, stepIntervalMs: 200 }
  }
  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): FadeCreateDraft {
    return { ...this.createDefaultConfig(), ...common, typeName: this.typeName }
  }
  createEditDraft(current: DeviceRecord): FadeCreateDraft {
    return { ...this.normalizeConfig(current.config, current.config.deps), typeName: this.typeName }
  }
  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): FadeAnalogOutputConfigDraft {
    const defaults = this.createDefaultConfig()
    if (!isRecord(value)) return defaults
    const links = depsFrom(value, deps)
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      deps: links,
      targetDeviceId: deviceId(value.targetDeviceId ?? firstTarget(links)),
      maxStep: percent(value.maxStep, defaults.maxStep),
      stepIntervalMs: Math.max(1, Math.round(Number(value.stepIntervalMs) || defaults.stepIntervalMs)),
    }
  }
  normalizeOutput(record: DeviceRecord): FadeAnalogOutputOutputSnapshot {
    return ((record.runtime as typeof record.runtime & { output?: FadeAnalogOutputOutputSnapshot }).output ?? {})
  }
  protected override encodeConfig(config: FadeAnalogOutputConfigDraft): Record<string, unknown> {
    return { ...encodeBaseDeviceConfig(config), maxStep: config.maxStep, stepIntervalMs: config.stepIntervalMs }
  }
  protected override createCreateDeps(config: FadeAnalogOutputConfigDraft): DeviceDependencyLink[] {
    return targetDeps([config.targetDeviceId])
  }
}

export class ScheduledAnalogOutputDevice extends BaseDevice<ScheduledAnalogOutputConfigDraft, ScheduledCreateDraft, ScheduledAnalogOutputOutputSnapshot> {
  static readonly TYPE_ID = 22
  static readonly TYPE_NAME = 'scheduled_analog_output'
  readonly typeId = ScheduledAnalogOutputDevice.TYPE_ID
  readonly typeName = ScheduledAnalogOutputDevice.TYPE_NAME
  readonly dependencyRoles: DeviceRole[] = ['analog_output']

  createDefaultConfig(): ScheduledAnalogOutputConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      targetDeviceId: 0,
      points: [{ deleted: false, minuteOfDay: 0, state: 100 }],
    }
  }
  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): ScheduledCreateDraft {
    return { ...this.createDefaultConfig(), ...common, typeName: this.typeName }
  }
  createEditDraft(current: DeviceRecord): ScheduledCreateDraft {
    return { ...this.normalizeConfig(current.config, current.config.deps), typeName: this.typeName }
  }
  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): ScheduledAnalogOutputConfigDraft {
    const defaults = this.createDefaultConfig()
    if (!isRecord(value)) return defaults
    const links = depsFrom(value, deps)
    const points = Array.isArray(value.points)
      ? value.points.filter(isRecord).slice(0, 10).map(point => ({
          deleted: point.deleted === true,
          minuteOfDay: Math.min(1439, Math.max(0, Math.round(Number(point.minuteOfDay) || 0))),
          state: percent(point.state),
        }))
      : defaults.points
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      deps: links,
      targetDeviceId: deviceId(value.targetDeviceId ?? firstTarget(links)),
      points: points.length > 0 ? points : defaults.points,
    }
  }
  normalizeOutput(record: DeviceRecord): ScheduledAnalogOutputOutputSnapshot {
    return ((record.runtime as typeof record.runtime & { output?: ScheduledAnalogOutputOutputSnapshot }).output ?? {})
  }
  protected override encodeConfig(config: ScheduledAnalogOutputConfigDraft): Record<string, unknown> {
    return { ...encodeBaseDeviceConfig(config), points: config.points.map(point => ({ ...point })) }
  }
  protected override createCreateDeps(config: ScheduledAnalogOutputConfigDraft): DeviceDependencyLink[] {
    return targetDeps([config.targetDeviceId])
  }
}

export class AnalogOutputComposerDevice extends BaseDevice<AnalogOutputComposerConfigDraft, ComposerCreateDraft, AnalogOutputComposerOutputSnapshot> {
  static readonly TYPE_ID = 23
  static readonly TYPE_NAME = 'analog_output_composer'
  readonly typeId = AnalogOutputComposerDevice.TYPE_ID
  readonly typeName = AnalogOutputComposerDevice.TYPE_NAME
  readonly dependencyRoles: DeviceRole[] = ['analog_output_group']

  createDefaultConfig(): AnalogOutputComposerConfigDraft {
    return { ...defaultBaseDeviceConfig(), targetDeviceIds: [] }
  }
  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): ComposerCreateDraft {
    return { ...this.createDefaultConfig(), ...common, typeName: this.typeName }
  }
  createEditDraft(current: DeviceRecord): ComposerCreateDraft {
    return { ...this.normalizeConfig(current.config, current.config.deps), typeName: this.typeName }
  }
  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): AnalogOutputComposerConfigDraft {
    const defaults = this.createDefaultConfig()
    if (!isRecord(value)) return defaults
    const links = depsFrom(value, deps)
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      deps: links,
      targetDeviceIds: links.filter(link => link.role === 'analog_output').map(link => link.deviceId),
    }
  }
  normalizeOutput(record: DeviceRecord): AnalogOutputComposerOutputSnapshot {
    return ((record.runtime as typeof record.runtime & { output?: AnalogOutputComposerOutputSnapshot }).output ?? {})
  }
  protected override encodeConfig(config: AnalogOutputComposerConfigDraft): Record<string, unknown> {
    return encodeBaseDeviceConfig(config)
  }
  protected override createCreateDeps(config: AnalogOutputComposerConfigDraft): DeviceDependencyLink[] {
    return targetDeps(config.targetDeviceIds)
  }
}
