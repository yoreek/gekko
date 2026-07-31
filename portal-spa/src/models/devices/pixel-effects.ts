import type {
  BaseDeviceConfig,
  DeviceDependencyLink,
  DeviceRecord,
  PixelColor,
  PixelEffectAlertOutputSnapshot,
  PixelEffectSolidOutputSnapshot,
} from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'
import type { DeviceCreateDraftBase } from './base.ts'
import { BaseDevice, defaultBaseDeviceConfig, encodeBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'

const isRecord = (value: unknown): value is Record<string, unknown> =>
  typeof value === 'object' && value !== null && !Array.isArray(value)
const deviceId = (value: unknown): number => {
  const number = Number(value)
  return Number.isFinite(number) && number > 0 ? Math.round(number) : 0
}
const clampChannel = (value: unknown, fallback: number): number => {
  const number = Number(value)
  return Number.isFinite(number) ? Math.min(255, Math.max(0, Math.round(number))) : fallback
}
const normalizeColor = (value: unknown, fallback: PixelColor): PixelColor =>
  isRecord(value)
    ? { r: clampChannel(value.r, fallback.r), g: clampChannel(value.g, fallback.g), b: clampChannel(value.b, fallback.b) }
    : fallback
const depsFrom = (raw: Record<string, unknown>, deps?: DeviceDependencyLink[]): DeviceDependencyLink[] =>
  Array.isArray(raw.deps) ? (raw.deps as DeviceDependencyLink[]) : deps ?? []
const firstStripTarget = (deps: DeviceDependencyLink[]): number => deps.find(link => link.role === 'pixel_strip')?.deviceId ?? 0

// ---- pixel_effect_solid ----

export interface PixelEffectSolidConfigDraft extends BaseDeviceConfig {
  targetDeviceId: number
  // Whether to power up at the last live color (retained state) or always at startupColor --
  // mirrors analog_output's restorePreviousState.
  restorePreviousState: boolean
  // Applied only at startup (or when no retained state is available). The live, currently-shown
  // color is runtime state set via the SetOutput command (see PixelEffectSolidWidget's live color
  // picker) and reported in runtime.output.color, never this field -- mirrors analog_output's
  // startupState/currentOutputState split (docs/pixel-strip.md).
  startupColor: PixelColor
}

export type PixelEffectSolidCreateDraft = DeviceCreateDraftBase & PixelEffectSolidConfigDraft

export class PixelEffectSolidDevice extends BaseDevice<
  PixelEffectSolidConfigDraft,
  PixelEffectSolidCreateDraft,
  PixelEffectSolidOutputSnapshot
> {
  static readonly TYPE_ID = 37
  static readonly TYPE_NAME = 'pixel_effect_solid' as const
  readonly typeId = PixelEffectSolidDevice.TYPE_ID
  readonly typeName = PixelEffectSolidDevice.TYPE_NAME
  readonly dependencyRoles: DeviceRole[] = ['pixel_strip']

  createDefaultConfig(): PixelEffectSolidConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      targetDeviceId: 0,
      restorePreviousState: false,
      startupColor: { r: 0, g: 0, b: 0 },
    }
  }
  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): PixelEffectSolidCreateDraft {
    return { ...this.createDefaultConfig(), ...common, typeName: this.typeName }
  }
  createEditDraft(current: DeviceRecord): PixelEffectSolidCreateDraft {
    return { ...this.normalizeConfig(current.config, current.config.deps), typeName: this.typeName }
  }
  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): PixelEffectSolidConfigDraft {
    const defaults = this.createDefaultConfig()
    if (!isRecord(value)) return defaults
    const links = depsFrom(value, deps)
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      deps: links,
      targetDeviceId: deviceId(value.targetDeviceId ?? firstStripTarget(links)),
      restorePreviousState: typeof value.restorePreviousState === 'boolean' ? value.restorePreviousState : defaults.restorePreviousState,
      startupColor: normalizeColor(value.startupColor, defaults.startupColor),
    }
  }
  normalizeOutput(record: DeviceRecord): PixelEffectSolidOutputSnapshot {
    return (record.runtime as typeof record.runtime & { output?: PixelEffectSolidOutputSnapshot }).output ?? {}
  }
  protected override encodeConfig(config: PixelEffectSolidConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      restorePreviousState: config.restorePreviousState,
      startupColor: { ...config.startupColor },
    }
  }
  protected override createCreateDeps(config: PixelEffectSolidConfigDraft): DeviceDependencyLink[] {
    return config.targetDeviceId > 0 ? [{ role: 'pixel_strip', deviceId: config.targetDeviceId }] : []
  }
}

// ---- pixel_effect_alert ----

export interface PixelEffectAlertCondition {
  deviceId: number
  invert: boolean
}

// Mirrors PixelEffectAlertDeviceConfig.h's kMaxPixelEffectAlertConditions.
export const PIXEL_EFFECT_ALERT_MAX_CONDITIONS = 4

export interface PixelEffectAlertConfigDraft extends BaseDeviceConfig {
  targetDeviceId: number
  color: PixelColor
  blinkIntervalMs: number
  conditions: PixelEffectAlertCondition[]
}

export type PixelEffectAlertCreateDraft = DeviceCreateDraftBase & PixelEffectAlertConfigDraft

function conditionsFromDeps(deps: DeviceDependencyLink[] | undefined): PixelEffectAlertCondition[] {
  return (deps ?? [])
    .filter(dep => dep.role === 'condition')
    .slice(0, PIXEL_EFFECT_ALERT_MAX_CONDITIONS)
    .map(dep => ({ deviceId: dep.deviceId, invert: dep.invert ?? false }))
}

function normalizeConditions(value: unknown, fallback: PixelEffectAlertCondition[]): PixelEffectAlertCondition[] {
  if (!Array.isArray(value)) return fallback
  return value
    .filter((entry): entry is Record<string, unknown> => isRecord(entry))
    .map(entry => ({ deviceId: deviceId(entry.deviceId), invert: entry.invert === true }))
    .filter(entry => entry.deviceId > 0)
    .slice(0, PIXEL_EFFECT_ALERT_MAX_CONDITIONS)
}

export class PixelEffectAlertDevice extends BaseDevice<
  PixelEffectAlertConfigDraft,
  PixelEffectAlertCreateDraft,
  PixelEffectAlertOutputSnapshot
> {
  static readonly TYPE_ID = 38
  static readonly TYPE_NAME = 'pixel_effect_alert' as const
  readonly typeId = PixelEffectAlertDevice.TYPE_ID
  readonly typeName = PixelEffectAlertDevice.TYPE_NAME
  readonly dependencyRoles: DeviceRole[] = ['pixel_strip', 'condition']

  createDefaultConfig(): PixelEffectAlertConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      targetDeviceId: 0,
      color: { r: 255, g: 0, b: 0 },
      blinkIntervalMs: 500,
      conditions: [],
    }
  }
  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): PixelEffectAlertCreateDraft {
    return { ...this.createDefaultConfig(), ...common, typeName: this.typeName }
  }
  createEditDraft(current: DeviceRecord): PixelEffectAlertCreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }
  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): PixelEffectAlertConfigDraft {
    const defaults = this.createDefaultConfig()
    if (!isRecord(value)) {
      return {
        ...defaults,
        deps: Array.isArray(deps) ? deps : defaults.deps,
        targetDeviceId: firstStripTarget(deps ?? []),
        conditions: conditionsFromDeps(deps),
      }
    }
    const links = depsFrom(value, deps)
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      deps: links,
      targetDeviceId: deviceId(value.targetDeviceId ?? firstStripTarget(links)),
      color: normalizeColor(value.color, defaults.color),
      blinkIntervalMs: Math.max(100, Math.min(60000, Math.round(Number(value.blinkIntervalMs) || defaults.blinkIntervalMs))),
      conditions: normalizeConditions(value.conditions, conditionsFromDeps(links)),
    }
  }
  normalizeOutput(record: DeviceRecord): PixelEffectAlertOutputSnapshot {
    return (record.runtime as typeof record.runtime & { output?: PixelEffectAlertOutputSnapshot }).output ?? {}
  }
  protected override encodeConfig(config: PixelEffectAlertConfigDraft): Record<string, unknown> {
    return { ...encodeBaseDeviceConfig(config), color: { ...config.color }, blinkIntervalMs: config.blinkIntervalMs }
  }
  protected override createCreateDeps(config: PixelEffectAlertConfigDraft): DeviceDependencyLink[] {
    const links: DeviceDependencyLink[] = []
    if (config.targetDeviceId > 0) {
      links.push({ role: 'pixel_strip', deviceId: config.targetDeviceId })
    }
    for (const condition of config.conditions.slice(0, PIXEL_EFFECT_ALERT_MAX_CONDITIONS)) {
      if (condition.deviceId > 0) {
        links.push({ role: 'condition', deviceId: condition.deviceId, invert: condition.invert })
      }
    }
    return links
  }
}
