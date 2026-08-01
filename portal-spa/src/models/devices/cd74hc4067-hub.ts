import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import type { AnalogInputHubOutputSnapshot, BaseDeviceConfig, DeviceRecord } from '@/api/contracts'
import type { AdcAttenuation } from './analog-port-input.ts'
import type { DeviceRole } from '@/models/device-type-ids'
import { normalizePin } from './shared/pin.ts'

export interface Cd74hc4067HubConfigDraft extends BaseDeviceConfig {
  selectPins: [number, number, number, number]
  enablePin: number
  sigPin: number
  sigAttenuation: AdcAttenuation
}

export interface Cd74hc4067HubCreateDraft extends DeviceCreateDraftBase, Cd74hc4067HubConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeSelectPins(value: unknown, defaults: [number, number, number, number]): [number, number, number, number] {
  if (!Array.isArray(value) || value.length !== 4) {
    return defaults
  }
  return value.map((pin, index) => normalizePin(pin, defaults[index], 'output')) as [number, number, number, number]
}

// 0xFF = not wired (tied to GND on the module), matching kGpioPinUnset in firmware.
export const CD74HC4067_UNUSED_PIN = 0xff

// A CD74HC4067 16-channel analog multiplexer, presented as an AnalogInputHub -- no bus
// dependency, it owns its GPIO pins directly (see analog-input-channel.ts for the per-channel leaf
// that depends on this hub).
export class Cd74hc4067HubDevice extends BaseDevice<Cd74hc4067HubConfigDraft, Cd74hc4067HubCreateDraft, AnalogInputHubOutputSnapshot> {
  static readonly TYPE_ID = 27 as const
  static readonly TYPE_NAME = 'cd74hc4067_hub' as const
  static readonly CHANNEL_COUNT = 16

  readonly typeName = Cd74hc4067HubDevice.TYPE_NAME
  readonly typeId = Cd74hc4067HubDevice.TYPE_ID
  readonly dependencyRoles: DeviceRole[] = ['analog_input_hub']

  static defaultConfig(): Cd74hc4067HubConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      selectPins: [16, 17, 18, 19],
      enablePin: CD74HC4067_UNUSED_PIN,
      sigPin: 255,
      sigAttenuation: '11db',
    }
  }

  static normalizeConfig(value: unknown): Cd74hc4067HubConfigDraft {
    const defaults = Cd74hc4067HubDevice.defaultConfig()
    if (!isRecord(value)) {
      return { ...defaults }
    }
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      selectPins: normalizeSelectPins(value.selectPins, defaults.selectPins),
      enablePin: normalizePin(value.enablePin, defaults.enablePin, 'output'),
      sigPin: normalizePin(value.sigPin, defaults.sigPin, 'adc1'),
      sigAttenuation: (['0db', '2_5db', '6db', '11db'] as AdcAttenuation[]).includes(value.sigAttenuation as AdcAttenuation)
        ? (value.sigAttenuation as AdcAttenuation)
        : defaults.sigAttenuation,
    }
  }

  static encodeConfig(config: Cd74hc4067HubConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      selectPins: [...config.selectPins],
      enablePin: config.enablePin,
      sigPin: config.sigPin,
      sigAttenuation: config.sigAttenuation,
    }
  }

  createDefaultConfig(): Cd74hc4067HubConfigDraft {
    return Cd74hc4067HubDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Cd74hc4067HubCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): Cd74hc4067HubCreateDraft {
    return {
      ...this.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown): Cd74hc4067HubConfigDraft {
    return Cd74hc4067HubDevice.normalizeConfig(value)
  }

  normalizeOutput(_record: DeviceRecord): AnalogInputHubOutputSnapshot {
    return {}
  }

  protected override encodeConfig(config: Cd74hc4067HubConfigDraft): Record<string, unknown> {
    return Cd74hc4067HubDevice.encodeConfig(config)
  }
}
