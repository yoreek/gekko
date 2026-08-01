import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import type { BaseDeviceConfig, DeviceRecord, RtcDs1302OutputSnapshot } from '@/api/contracts'
import { normalizePin } from './shared/pin.ts'

export interface RtcDs1302ConfigDraft extends BaseDeviceConfig {
  clkPin: number
  dataPin: number
  rstPin: number
  useForSystemTimeSync: boolean
}

export interface RtcDs1302CreateDraft extends DeviceCreateDraftBase, RtcDs1302ConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

export class RtcDs1302Device extends BaseDevice<RtcDs1302ConfigDraft, RtcDs1302CreateDraft, RtcDs1302OutputSnapshot> {
  static readonly TYPE_ID = 32 as const
  static readonly TYPE_NAME = 'rtc_ds1302' as const

  readonly typeName = RtcDs1302Device.TYPE_NAME
  readonly typeId = RtcDs1302Device.TYPE_ID

  static defaultConfig(): RtcDs1302ConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      clkPin: 255,
      dataPin: 255,
      rstPin: 255,
      useForSystemTimeSync: false,
    }
  }

  static normalizeConfig(value: unknown): RtcDs1302ConfigDraft {
    const defaults = RtcDs1302Device.defaultConfig()
    if (!isRecord(value)) {
      return defaults
    }
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      deps: [],
      clkPin: normalizePin(value.clkPin, defaults.clkPin, 'output'),
      dataPin: normalizePin(value.dataPin, defaults.dataPin, 'output'),
      rstPin: normalizePin(value.rstPin, defaults.rstPin, 'output'),
      useForSystemTimeSync: typeof value.useForSystemTimeSync === 'boolean' ? value.useForSystemTimeSync : defaults.useForSystemTimeSync,
    }
  }

  static encodeConfig(config: RtcDs1302ConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      deps: [],
      clkPin: config.clkPin,
      dataPin: config.dataPin,
      rstPin: config.rstPin,
      useForSystemTimeSync: config.useForSystemTimeSync,
    }
  }

  createDefaultConfig(): RtcDs1302ConfigDraft {
    return RtcDs1302Device.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): RtcDs1302CreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): RtcDs1302CreateDraft {
    return {
      ...this.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown): RtcDs1302ConfigDraft {
    return RtcDs1302Device.normalizeConfig(value)
  }

  normalizeOutput(record: DeviceRecord): RtcDs1302OutputSnapshot {
    return record.runtime as RtcDs1302OutputSnapshot
  }

  protected override encodeConfig(config: RtcDs1302ConfigDraft): Record<string, unknown> {
    return RtcDs1302Device.encodeConfig(config)
  }
}
