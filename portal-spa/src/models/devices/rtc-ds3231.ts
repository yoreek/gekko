import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import type { BaseDeviceConfig, DeviceDependencyLink, DeviceRecord, RtcDs3231OutputSnapshot } from '@/api/contracts'

export interface RtcDs3231ConfigDraft extends BaseDeviceConfig {
  dependencyDeviceId: number
  i2cAddress: number
  useForSystemTimeSync: boolean
}

export interface RtcDs3231CreateDraft extends DeviceCreateDraftBase, RtcDs3231ConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeDependencyDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

export class RtcDs3231Device extends BaseDevice<RtcDs3231ConfigDraft, RtcDs3231CreateDraft, RtcDs3231OutputSnapshot> {
  static readonly TYPE_ID = 11 as const
  static readonly TYPE_NAME = 'rtc_ds3231' as const

  readonly typeName = RtcDs3231Device.TYPE_NAME
  readonly typeId = RtcDs3231Device.TYPE_ID

  static defaultConfig(): RtcDs3231ConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      dependencyDeviceId: 0,
      i2cAddress: 0x68,
      useForSystemTimeSync: false,
    }
  }

  static normalizeConfig(value: unknown, dependencyDeviceOrDeps?: number | DeviceDependencyLink[]): RtcDs3231ConfigDraft {
    const defaults = RtcDs3231Device.defaultConfig()
    const dependencyDeviceId = Array.isArray(dependencyDeviceOrDeps)
      ? (dependencyDeviceOrDeps.find(dep => dep.role === 'i2c_bus')?.deviceId ?? 0)
      : typeof dependencyDeviceOrDeps === 'number'
        ? dependencyDeviceOrDeps
        : 0
    if (!isRecord(value)) {
      return {
        ...defaults,
        dependencyDeviceId: normalizeDependencyDeviceId(dependencyDeviceId),
        deps: Array.isArray(dependencyDeviceOrDeps) ? dependencyDeviceOrDeps : defaults.deps,
      }
    }
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      dependencyDeviceId: normalizeDependencyDeviceId(dependencyDeviceId ?? value.dependencyDeviceId),
      i2cAddress: typeof value.i2cAddress === 'number' ? value.i2cAddress : defaults.i2cAddress,
      useForSystemTimeSync: typeof value.useForSystemTimeSync === 'boolean' ? value.useForSystemTimeSync : defaults.useForSystemTimeSync,
    }
  }

  static encodeConfig(config: RtcDs3231ConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      i2cAddress: config.i2cAddress,
      useForSystemTimeSync: config.useForSystemTimeSync,
    }
  }

  createDefaultConfig(): RtcDs3231ConfigDraft {
    return RtcDs3231Device.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): RtcDs3231CreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): RtcDs3231CreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): RtcDs3231ConfigDraft {
    return RtcDs3231Device.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): RtcDs3231OutputSnapshot {
    return record.runtime as RtcDs3231OutputSnapshot
  }

  protected override encodeConfig(config: RtcDs3231ConfigDraft): Record<string, unknown> {
    return RtcDs3231Device.encodeConfig(config)
  }

  protected override createCreateDeps(config: RtcDs3231ConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'i2c_bus',
        deviceId: config.dependencyDeviceId,
      },
    ]
  }
}
