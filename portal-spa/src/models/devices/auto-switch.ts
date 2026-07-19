import type { AutoSwitchMode, AutoSwitchOutputSnapshot, BaseDeviceConfig, DeviceDependencyLink } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceRecord } from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'

export interface AutoSwitchCondition {
  deviceId: number
  invert: boolean
}

// Mirrors AutoSwitchDeviceConfig.h's kMaxAutoSwitchConditions.
export const AUTO_SWITCH_MAX_CONDITIONS = 6

export interface AutoSwitchConfigDraft extends BaseDeviceConfig {
  pauseDurationSeconds: number
  targetSwitchDeviceId: number
  conditions: AutoSwitchCondition[]
}

export interface AutoSwitchCreateDraft extends DeviceCreateDraftBase, AutoSwitchConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

function normalizePauseDurationSeconds(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 1 ? Math.round(numeric) : fallback
}

function deviceIdFromDeps(deps: DeviceDependencyLink[] | undefined, role: DeviceRole): number {
  return deps?.find(dep => dep.role === role)?.deviceId ?? 0
}

function conditionsFromDeps(deps: DeviceDependencyLink[] | undefined): AutoSwitchCondition[] {
  return (deps ?? [])
    .filter(dep => dep.role === 'condition')
    .slice(0, AUTO_SWITCH_MAX_CONDITIONS)
    .map(dep => ({ deviceId: dep.deviceId, invert: dep.invert ?? false }))
}

function normalizeConditions(value: unknown, fallback: AutoSwitchCondition[]): AutoSwitchCondition[] {
  if (!Array.isArray(value)) {
    return fallback
  }
  return value
    .filter((entry): entry is Record<string, unknown> => isRecord(entry))
    .map(entry => ({ deviceId: normalizeDeviceId(entry.deviceId), invert: entry.invert === true }))
    .filter(entry => entry.deviceId > 0)
    .slice(0, AUTO_SWITCH_MAX_CONDITIONS)
}

export class AutoSwitchDevice extends BaseDevice<AutoSwitchConfigDraft, AutoSwitchCreateDraft, AutoSwitchOutputSnapshot> {
  static readonly TYPE_ID = 16 as const
  static readonly TYPE_NAME = 'auto_switch' as const

  readonly typeName = AutoSwitchDevice.TYPE_NAME
  readonly typeId = AutoSwitchDevice.TYPE_ID
  // AutoSwitch provides DeviceRole::Switch (it wraps a target switch and behaves like one) and
  // DeviceRole::Condition (so another AutoSwitch can chain it as an AND-condition source).
  readonly dependencyRoles: DeviceRole[] = ['switch', 'condition']

  static defaultConfig(): AutoSwitchConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      pauseDurationSeconds: 3600,
      targetSwitchDeviceId: 0,
      conditions: [],
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): AutoSwitchConfigDraft {
    const defaults = AutoSwitchDevice.defaultConfig()
    if (!isRecord(value)) {
      return {
        ...defaults,
        deps: Array.isArray(deps) ? deps : defaults.deps,
        targetSwitchDeviceId: deviceIdFromDeps(deps, 'switch'),
        conditions: conditionsFromDeps(deps),
      }
    }

    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      pauseDurationSeconds: normalizePauseDurationSeconds(value.pauseDurationSeconds, defaults.pauseDurationSeconds),
      targetSwitchDeviceId: normalizeDeviceId(value.targetSwitchDeviceId ?? deviceIdFromDeps(deps, 'switch')),
      conditions: normalizeConditions(value.conditions, conditionsFromDeps(deps)),
    }
  }

  static dependencyLinks(config: AutoSwitchConfigDraft): DeviceDependencyLink[] {
    const links: DeviceDependencyLink[] = [{ role: 'switch', deviceId: config.targetSwitchDeviceId }]
    for (const condition of config.conditions.slice(0, AUTO_SWITCH_MAX_CONDITIONS)) {
      if (condition.deviceId > 0) {
        links.push({ role: 'condition', deviceId: condition.deviceId, invert: condition.invert })
      }
    }
    return links
  }

  static modeLabelKey(mode: AutoSwitchMode): string {
    return `device.dialog.autoSwitch.mode.${mode}`
  }

  createDefaultConfig(): AutoSwitchConfigDraft {
    return AutoSwitchDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): AutoSwitchCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): AutoSwitchCreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): AutoSwitchConfigDraft {
    return AutoSwitchDevice.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): AutoSwitchOutputSnapshot {
    return record.runtime as AutoSwitchOutputSnapshot
  }

  protected override createCreateDeps(config: AutoSwitchConfigDraft): DeviceDependencyLink[] {
    return AutoSwitchDevice.dependencyLinks(config)
  }
}
