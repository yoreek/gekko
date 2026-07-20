import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import type { AnalogInputHubOutputSnapshot, BaseDeviceConfig, DeviceDependencyLink, DeviceRecord } from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'

export type Ads1115Gain = 'fsr6144' | 'fsr4096' | 'fsr2048' | 'fsr1024' | 'fsr0512' | 'fsr0256'
export type Ads1115DataRate = '8' | '16' | '32' | '64' | '128' | '250' | '475' | '860'

export interface Ads1115HubConfigDraft extends BaseDeviceConfig {
  dependencyDeviceId: number
  i2cAddress: number
  gain: Ads1115Gain
  dataRateSps: Ads1115DataRate
}

export interface Ads1115HubCreateDraft extends DeviceCreateDraftBase, Ads1115HubConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeDependencyDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

// An ADS1115 16-bit I2C ADC, presented as an AnalogInputHub with 4 single-ended channels
// (see analog-input-channel.ts for the per-channel leaf that depends on this hub).
export class Ads1115HubDevice extends BaseDevice<Ads1115HubConfigDraft, Ads1115HubCreateDraft, AnalogInputHubOutputSnapshot> {
  static readonly TYPE_ID = 25 as const
  static readonly TYPE_NAME = 'ads1115_hub' as const
  static readonly CHANNEL_COUNT = 4
  static readonly gainOptions: Ads1115Gain[] = ['fsr6144', 'fsr4096', 'fsr2048', 'fsr1024', 'fsr0512', 'fsr0256']
  static readonly dataRateOptions: Ads1115DataRate[] = ['8', '16', '32', '64', '128', '250', '475', '860']

  readonly typeName = Ads1115HubDevice.TYPE_NAME
  readonly typeId = Ads1115HubDevice.TYPE_ID
  readonly dependencyRoles: DeviceRole[] = ['analog_input_hub']

  static defaultConfig(): Ads1115HubConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      dependencyDeviceId: 0,
      i2cAddress: 0x48,
      gain: 'fsr2048',
      dataRateSps: '128',
    }
  }

  static normalizeConfig(value: unknown, dependencyDeviceOrDeps?: number | DeviceDependencyLink[]): Ads1115HubConfigDraft {
    const defaults = Ads1115HubDevice.defaultConfig()
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
      i2cAddress: typeof value.i2cAddress === 'number' && Number.isFinite(value.i2cAddress) ? value.i2cAddress : defaults.i2cAddress,
      gain: Ads1115HubDevice.gainOptions.includes(value.gain as Ads1115Gain) ? (value.gain as Ads1115Gain) : defaults.gain,
      dataRateSps: Ads1115HubDevice.dataRateOptions.includes(value.dataRateSps as Ads1115DataRate)
        ? (value.dataRateSps as Ads1115DataRate)
        : defaults.dataRateSps,
    }
  }

  static encodeConfig(config: Ads1115HubConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      i2cAddress: config.i2cAddress,
      gain: config.gain,
      dataRateSps: config.dataRateSps,
    }
  }

  createDefaultConfig(): Ads1115HubConfigDraft {
    return Ads1115HubDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Ads1115HubCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): Ads1115HubCreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Ads1115HubConfigDraft {
    return Ads1115HubDevice.normalizeConfig(value, deps)
  }

  normalizeOutput(_record: DeviceRecord): AnalogInputHubOutputSnapshot {
    return {}
  }

  protected override encodeConfig(config: Ads1115HubConfigDraft): Record<string, unknown> {
    return Ads1115HubDevice.encodeConfig(config)
  }

  protected override createCreateDeps(config: Ads1115HubConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'i2c_bus',
        deviceId: config.dependencyDeviceId,
      },
    ]
  }
}
