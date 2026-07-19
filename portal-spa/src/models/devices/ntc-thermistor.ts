import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import { TemperatureSensorDevice } from './temperature-sensor-device.ts'
import { defaultSensorFilterConfig, normalizeSensorFilterConfig, type SensorFilterConfig } from './sensor-filter.ts'
import type {
  BaseDeviceConfig,
  DeviceRecord,
  NtcThermistorTemperatureSensorOutputSnapshot,
  TemperatureUnit,
} from '@/api/contracts'

export type NtcAttenuation = '0db' | '2_5db' | '6db' | '11db'

export interface NtcThermistorConfigDraft extends BaseDeviceConfig {
  gpioPin: number
  attenuation: NtcAttenuation
  seriesResistorOhms: number
  nominalResistanceOhms: number
  nominalTempCelsius: number
  betaCoefficient: number
  adcSamples: number
  unit: TemperatureUnit
  pollMs: number
  reportDeltaCelsius: number
  reportAlways: boolean
  filter: SensorFilterConfig
}

export interface NtcThermistorCreateDraft extends DeviceCreateDraftBase, NtcThermistorConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

export class NtcThermistorDevice extends TemperatureSensorDevice<
  NtcThermistorConfigDraft,
  NtcThermistorCreateDraft,
  NtcThermistorTemperatureSensorOutputSnapshot
> {
  static readonly TYPE_ID = 10 as const
  static readonly TYPE_NAME = 'ntc_thermistor_temperature_sensor' as const
  static readonly attenuationOptions: NtcAttenuation[] = ['0db', '2_5db', '6db', '11db']

  readonly typeName = NtcThermistorDevice.TYPE_NAME
  readonly typeId = NtcThermistorDevice.TYPE_ID

  static defaultConfig(): NtcThermistorConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      gpioPin: 34,
      attenuation: '11db',
      seriesResistorOhms: 10000,
      nominalResistanceOhms: 100000,
      nominalTempCelsius: 25,
      betaCoefficient: 3950,
      adcSamples: 8,
      unit: 'celsius',
      pollMs: 5000,
      reportDeltaCelsius: 0.1,
      reportAlways: false,
      filter: defaultSensorFilterConfig(),
    }
  }

  static normalizeConfig(value: unknown): NtcThermistorConfigDraft {
    const defaults = NtcThermistorDevice.defaultConfig()
    if (!isRecord(value)) {
      return { ...defaults }
    }
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      gpioPin: typeof value.gpioPin === 'number' && Number.isFinite(value.gpioPin) ? value.gpioPin : defaults.gpioPin,
      attenuation: NtcThermistorDevice.attenuationOptions.includes(value.attenuation as NtcAttenuation)
        ? (value.attenuation as NtcAttenuation)
        : defaults.attenuation,
      seriesResistorOhms: typeof value.seriesResistorOhms === 'number' && Number.isFinite(value.seriesResistorOhms)
        ? value.seriesResistorOhms
        : defaults.seriesResistorOhms,
      nominalResistanceOhms: typeof value.nominalResistanceOhms === 'number' && Number.isFinite(value.nominalResistanceOhms)
        ? value.nominalResistanceOhms
        : defaults.nominalResistanceOhms,
      nominalTempCelsius: typeof value.nominalTempCelsius === 'number' && Number.isFinite(value.nominalTempCelsius)
        ? value.nominalTempCelsius
        : defaults.nominalTempCelsius,
      betaCoefficient: typeof value.betaCoefficient === 'number' && Number.isFinite(value.betaCoefficient)
        ? value.betaCoefficient
        : defaults.betaCoefficient,
      adcSamples: typeof value.adcSamples === 'number' && Number.isFinite(value.adcSamples) ? value.adcSamples : defaults.adcSamples,
      unit: NtcThermistorDevice.temperatureUnitOptions.includes(value.unit as TemperatureUnit)
        ? (value.unit as TemperatureUnit)
        : defaults.unit,
      pollMs: typeof value.pollMs === 'number' && Number.isFinite(value.pollMs) ? value.pollMs : defaults.pollMs,
      reportDeltaCelsius: typeof value.reportDeltaCelsius === 'number' && Number.isFinite(value.reportDeltaCelsius)
        ? value.reportDeltaCelsius
        : defaults.reportDeltaCelsius,
      reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : defaults.reportAlways,
      filter: normalizeSensorFilterConfig(value, defaults.filter),
    }
  }

  static encodeConfig(config: NtcThermistorConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      gpioPin: config.gpioPin,
      attenuation: config.attenuation,
      seriesResistorOhms: config.seriesResistorOhms,
      nominalResistanceOhms: config.nominalResistanceOhms,
      nominalTempCelsius: config.nominalTempCelsius,
      betaCoefficient: config.betaCoefficient,
      adcSamples: config.adcSamples,
      unit: config.unit,
      pollMs: config.pollMs,
      reportDeltaCelsius: config.reportDeltaCelsius,
      reportAlways: config.reportAlways,
      smoothingWeight: config.filter.smoothingWeight,
      calibrationFactor: config.filter.calibrationFactor,
      calibrationOffset: config.filter.calibrationOffset,
    }
  }

  createDefaultConfig(): NtcThermistorConfigDraft {
    return NtcThermistorDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): NtcThermistorCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): NtcThermistorCreateDraft {
    return {
      ...this.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown): NtcThermistorConfigDraft {
    return NtcThermistorDevice.normalizeConfig(value)
  }

  normalizeOutput(record: DeviceRecord): NtcThermistorTemperatureSensorOutputSnapshot {
    return record.runtime as NtcThermistorTemperatureSensorOutputSnapshot
  }

  protected override encodeConfig(config: NtcThermistorConfigDraft): Record<string, unknown> {
    return NtcThermistorDevice.encodeConfig(config)
  }
}
