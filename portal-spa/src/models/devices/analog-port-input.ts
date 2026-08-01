import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import type { AnalogInputOutputSnapshot, BaseDeviceConfig, DeviceRecord } from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'
import { normalizePin } from './shared/pin.ts'

export type AdcAttenuation = '0db' | '2_5db' | '6db' | '11db'

export interface AnalogPortInputConfigDraft extends BaseDeviceConfig {
  gpioPin: number
  attenuation: AdcAttenuation
  adcSamples: number
  reportAlways: boolean
  reportDeltaMilliVolts: number
  pollMs: number
}

export interface AnalogPortInputCreateDraft extends DeviceCreateDraftBase, AnalogPortInputConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

// The ESP32's own ADC exposed as an AnalogInput-role device: reads one pin directly, no hub
// dependency (see analog-input-channel.ts for the hub-dependent leaf).
export class AnalogPortInputDevice extends BaseDevice<AnalogPortInputConfigDraft, AnalogPortInputCreateDraft, AnalogInputOutputSnapshot> {
  static readonly TYPE_ID = 24 as const
  static readonly TYPE_NAME = 'analog_port_input' as const
  static readonly attenuationOptions: AdcAttenuation[] = ['0db', '2_5db', '6db', '11db']

  readonly typeName = AnalogPortInputDevice.TYPE_NAME
  readonly typeId = AnalogPortInputDevice.TYPE_ID
  readonly dependencyRoles: DeviceRole[] = ['analog_input']

  static defaultConfig(): AnalogPortInputConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      gpioPin: 255,
      attenuation: '11db',
      adcSamples: 8,
      reportAlways: false,
      reportDeltaMilliVolts: 10,
      pollMs: 1000,
    }
  }

  static normalizeConfig(value: unknown): AnalogPortInputConfigDraft {
    const defaults = AnalogPortInputDevice.defaultConfig()
    if (!isRecord(value)) {
      return { ...defaults }
    }
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      gpioPin: normalizePin(value.gpioPin, defaults.gpioPin, 'adc1'),
      attenuation: AnalogPortInputDevice.attenuationOptions.includes(value.attenuation as AdcAttenuation)
        ? (value.attenuation as AdcAttenuation)
        : defaults.attenuation,
      adcSamples: typeof value.adcSamples === 'number' && Number.isFinite(value.adcSamples) ? value.adcSamples : defaults.adcSamples,
      reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : defaults.reportAlways,
      reportDeltaMilliVolts: typeof value.reportDeltaMilliVolts === 'number' && Number.isFinite(value.reportDeltaMilliVolts)
        ? value.reportDeltaMilliVolts
        : defaults.reportDeltaMilliVolts,
      pollMs: typeof value.pollMs === 'number' && Number.isFinite(value.pollMs) ? value.pollMs : defaults.pollMs,
    }
  }

  static encodeConfig(config: AnalogPortInputConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      gpioPin: config.gpioPin,
      attenuation: config.attenuation,
      adcSamples: config.adcSamples,
      reportAlways: config.reportAlways,
      reportDeltaMilliVolts: config.reportDeltaMilliVolts,
      pollMs: config.pollMs,
    }
  }

  createDefaultConfig(): AnalogPortInputConfigDraft {
    return AnalogPortInputDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): AnalogPortInputCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): AnalogPortInputCreateDraft {
    return {
      ...this.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown): AnalogPortInputConfigDraft {
    return AnalogPortInputDevice.normalizeConfig(value)
  }

  normalizeOutput(record: DeviceRecord): AnalogInputOutputSnapshot {
    return record.runtime as AnalogInputOutputSnapshot
  }

  protected override encodeConfig(config: AnalogPortInputConfigDraft): Record<string, unknown> {
    return AnalogPortInputDevice.encodeConfig(config)
  }
}
