import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import { Ads1115HubDevice } from './ads1115-hub.ts'
import { Cd74hc4067HubDevice } from './cd74hc4067-hub.ts'
import type { AnalogInputOutputSnapshot, BaseDeviceConfig, DeviceDependencyLink, DeviceRecord } from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'

// The channel's real max is whichever hub is actually wired up, not a static per-type constant --
// used both by the channel create/edit form (to bound the channel number input) and by
// AnalogInputHubWidget.vue (to show "N channels" for whichever hub type it's rendering).
export function analogInputHubChannelCount(hubTypeName: string | undefined): number {
  return hubTypeName === Cd74hc4067HubDevice.TYPE_NAME ? Cd74hc4067HubDevice.CHANNEL_COUNT : Ads1115HubDevice.CHANNEL_COUNT
}

// One channel of whatever hub is wired up via the generic 'analog_input_hub' dependency role --
// deliberately hub-agnostic (mirrors AnalogInputChannelDevice on the firmware side), so this is
// the only channel device type, not one per hub chip. The channel's real upper bound is whichever
// hub is actually selected (see analogInputHubChannelCount), not a static per-type constant.
export interface AnalogInputChannelConfigDraft extends BaseDeviceConfig {
  dependencyDeviceId: number
  channel: number
  adcSamples: number
  reportAlways: boolean
  reportDeltaMilliVolts: number
  pollMs: number
}

export interface AnalogInputChannelCreateDraft extends DeviceCreateDraftBase, AnalogInputChannelConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeDependencyDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

export function defaultAnalogInputChannelConfig(): AnalogInputChannelConfigDraft {
  return {
    ...defaultBaseDeviceConfig(),
    dependencyDeviceId: 0,
    channel: 0,
    adcSamples: 4,
    reportAlways: false,
    reportDeltaMilliVolts: 10,
    pollMs: 1000,
  }
}

export function normalizeAnalogInputChannelConfig(
  value: unknown,
  dependencyDeviceOrDeps?: number | DeviceDependencyLink[],
): AnalogInputChannelConfigDraft {
  const defaults = defaultAnalogInputChannelConfig()
  const dependencyDeviceId = Array.isArray(dependencyDeviceOrDeps)
    ? (dependencyDeviceOrDeps.find(dep => dep.role === 'analog_input_hub')?.deviceId ?? 0)
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
    channel: typeof value.channel === 'number' && Number.isFinite(value.channel) ? value.channel : defaults.channel,
    adcSamples: typeof value.adcSamples === 'number' && Number.isFinite(value.adcSamples) ? value.adcSamples : defaults.adcSamples,
    reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : defaults.reportAlways,
    reportDeltaMilliVolts: typeof value.reportDeltaMilliVolts === 'number' && Number.isFinite(value.reportDeltaMilliVolts)
      ? value.reportDeltaMilliVolts
      : defaults.reportDeltaMilliVolts,
    pollMs: typeof value.pollMs === 'number' && Number.isFinite(value.pollMs) ? value.pollMs : defaults.pollMs,
  }
}

export function encodeAnalogInputChannelConfig(config: AnalogInputChannelConfigDraft): Record<string, unknown> {
  return {
    ...encodeBaseDeviceConfig(config),
    channel: config.channel,
    adcSamples: config.adcSamples,
    reportAlways: config.reportAlways,
    reportDeltaMilliVolts: config.reportDeltaMilliVolts,
    pollMs: config.pollMs,
  }
}

export class AnalogInputChannelDevice extends BaseDevice<
  AnalogInputChannelConfigDraft,
  AnalogInputChannelCreateDraft,
  AnalogInputOutputSnapshot
> {
  static readonly TYPE_ID = 26 as const
  static readonly TYPE_NAME = 'analog_input_channel' as const

  readonly typeName = AnalogInputChannelDevice.TYPE_NAME
  readonly typeId = AnalogInputChannelDevice.TYPE_ID
  readonly dependencyRoles: DeviceRole[] = ['analog_input']

  createDefaultConfig(): AnalogInputChannelConfigDraft {
    return defaultAnalogInputChannelConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): AnalogInputChannelCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): AnalogInputChannelCreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): AnalogInputChannelConfigDraft {
    return normalizeAnalogInputChannelConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): AnalogInputOutputSnapshot {
    return record.runtime as AnalogInputOutputSnapshot
  }

  protected override encodeConfig(config: AnalogInputChannelConfigDraft): Record<string, unknown> {
    return encodeAnalogInputChannelConfig(config)
  }

  protected override createCreateDeps(config: AnalogInputChannelConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'analog_input_hub',
        deviceId: config.dependencyDeviceId,
      },
    ]
  }
}
