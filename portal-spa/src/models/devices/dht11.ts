import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import { TemperatureSensorDevice } from './temperature-sensor-device.ts'
import { defaultSensorFilterConfig, normalizeSensorFilterConfig, type SensorFilterConfig } from './sensor-filter.ts'
import type {
  BaseDeviceConfig,
  DeviceRecord,
  Dht11SensorOutputSnapshot,
  TemperatureUnit,
} from '@/api/contracts'

export interface Dht11ConfigDraft extends BaseDeviceConfig {
  gpioPin: number
  internalPullup: boolean
  captureMode: 'native' | 'rmt'
  unit: TemperatureUnit
  pollMs: number
  reportDeltaCelsius: number
  reportDeltaHumidity: number
  reportAlways: boolean
  temperatureFilter: SensorFilterConfig
  humidityFilter: SensorFilterConfig
}

export interface Dht11CreateDraft extends DeviceCreateDraftBase, Dht11ConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeGpioPin(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 0 && numeric <= 39 ? Math.round(numeric) : fallback
}

export class Dht11Device extends TemperatureSensorDevice<Dht11ConfigDraft, Dht11CreateDraft, Dht11SensorOutputSnapshot> {
  static readonly TYPE_ID = 31 as const
  static readonly TYPE_NAME = 'dht11' as const

  readonly typeName = Dht11Device.TYPE_NAME
  readonly typeId = Dht11Device.TYPE_ID

  static formatHumidity(output: Dht11SensorOutputSnapshot['humidity'] | undefined): string {
    if (!output?.valid) {
      return ''
    }
    return `${output.value.toFixed(1)} ${output.unitSymbol}`
  }

  static defaultConfig(): Dht11ConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      gpioPin: 17,
      internalPullup: false,
      captureMode: 'native',
      unit: 'celsius',
      pollMs: 5000,
      reportDeltaCelsius: 0.1,
      reportDeltaHumidity: 0.1,
      reportAlways: false,
      temperatureFilter: defaultSensorFilterConfig(),
      humidityFilter: defaultSensorFilterConfig(),
    }
  }

  static normalizeConfig(value: unknown): Dht11ConfigDraft {
    const defaults = Dht11Device.defaultConfig()
    if (!isRecord(value)) {
      return defaults
    }
    const reportDeltaCenti = typeof value.reportDeltaCentiCelsius === 'number' ? value.reportDeltaCentiCelsius / 100 : undefined
    const reportDeltaCentiHumidity = typeof value.reportDeltaCentiPercent === 'number' ? value.reportDeltaCentiPercent / 100 : undefined
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      deps: [],
      gpioPin: normalizeGpioPin(value.gpioPin, defaults.gpioPin),
      internalPullup: typeof value.internalPullup === 'boolean' ? value.internalPullup : defaults.internalPullup,
      captureMode: value.captureMode === 'rmt' ? 'rmt' : 'native',
      unit: TemperatureSensorDevice.temperatureUnitOptions.includes(value.unit as TemperatureUnit)
        ? (value.unit as TemperatureUnit)
        : defaults.unit,
      pollMs: typeof value.pollMs === 'number' && Number.isFinite(value.pollMs) ? value.pollMs : defaults.pollMs,
      reportDeltaCelsius: typeof value.reportDeltaCelsius === 'number' && Number.isFinite(value.reportDeltaCelsius)
        ? value.reportDeltaCelsius
        : reportDeltaCenti ?? defaults.reportDeltaCelsius,
      reportDeltaHumidity: typeof value.reportDeltaHumidity === 'number' && Number.isFinite(value.reportDeltaHumidity)
        ? value.reportDeltaHumidity
        : reportDeltaCentiHumidity ?? defaults.reportDeltaHumidity,
      reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : defaults.reportAlways,
      temperatureFilter: normalizeSensorFilterConfig(value.temperatureFilter, defaults.temperatureFilter),
      humidityFilter: normalizeSensorFilterConfig(value.humidityFilter, defaults.humidityFilter),
    }
  }

  static encodeConfig(config: Dht11ConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      deps: [],
      gpioPin: config.gpioPin,
      internalPullup: config.internalPullup,
      captureMode: config.captureMode,
      unit: config.unit,
      pollMs: config.pollMs,
      reportDeltaCelsius: config.reportDeltaCelsius,
      reportDeltaHumidity: config.reportDeltaHumidity,
      reportAlways: config.reportAlways,
      temperatureFilter: { ...config.temperatureFilter },
      humidityFilter: { ...config.humidityFilter },
    }
  }

  createDefaultConfig(): Dht11ConfigDraft {
    return Dht11Device.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Dht11CreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): Dht11CreateDraft {
    return {
      ...Dht11Device.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown): Dht11ConfigDraft {
    return Dht11Device.normalizeConfig(value)
  }

  normalizeOutput(record: DeviceRecord): Dht11SensorOutputSnapshot {
    return record.runtime as Dht11SensorOutputSnapshot
  }

  protected override encodeConfig(config: Dht11ConfigDraft): Record<string, unknown> {
    return Dht11Device.encodeConfig(config)
  }
}
