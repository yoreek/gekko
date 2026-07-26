import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import { TemperatureSensorDevice } from './temperature-sensor-device.ts'
import { defaultSensorFilterConfig, normalizeSensorFilterConfig, type SensorFilterConfig } from './sensor-filter.ts'
import type {
  BaseDeviceConfig,
  DeviceDependencyLink,
  DeviceRecord,
  Aht10SensorOutputSnapshot,
  HumidityOutputSnapshot,
  TemperatureUnit,
} from '@/api/contracts'

export interface Aht10ConfigDraft extends BaseDeviceConfig {
  dependencyDeviceId: number
  i2cAddress: number
  unit: TemperatureUnit
  pollMs: number
  reportDeltaCelsius: number
  reportDeltaHumidity: number
  reportAlways: boolean
  temperatureFilter: SensorFilterConfig
  humidityFilter: SensorFilterConfig
}

export interface Aht10CreateDraft extends DeviceCreateDraftBase, Aht10ConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeDependencyDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

export class Aht10Device extends TemperatureSensorDevice<Aht10ConfigDraft, Aht10CreateDraft, Aht10SensorOutputSnapshot> {
  static readonly TYPE_ID = 29 as const
  static readonly TYPE_NAME = 'aht10' as const

  readonly typeName = Aht10Device.TYPE_NAME
  readonly typeId = Aht10Device.TYPE_ID

  static formatHumidity(output: HumidityOutputSnapshot | undefined): string {
    if (!output?.valid) {
      return ''
    }
    return `${output.value.toFixed(1)} ${output.unitSymbol}`
  }

  static defaultConfig(): Aht10ConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      dependencyDeviceId: 0,
      i2cAddress: 0x38,
      unit: 'celsius',
      pollMs: 5000,
      reportDeltaCelsius: 0.1,
      reportDeltaHumidity: 0.1,
      reportAlways: false,
      temperatureFilter: defaultSensorFilterConfig(),
      humidityFilter: defaultSensorFilterConfig(),
    }
  }

  static normalizeConfig(value: unknown, dependencyDeviceOrDeps?: number | DeviceDependencyLink[]): Aht10ConfigDraft {
    const defaults = Aht10Device.defaultConfig()
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
    const reportDeltaCenti = typeof value.reportDeltaCentiCelsius === 'number' ? value.reportDeltaCentiCelsius / 100 : undefined
    const reportDeltaCentiHumidity = typeof value.reportDeltaCentiPercent === 'number' ? value.reportDeltaCentiPercent / 100 : undefined
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      dependencyDeviceId: normalizeDependencyDeviceId(dependencyDeviceId ?? value.dependencyDeviceId),
      i2cAddress: typeof value.i2cAddress === 'number' && Number.isFinite(value.i2cAddress)
        ? value.i2cAddress
        : defaults.i2cAddress,
      unit: Aht10Device.temperatureUnitOptions.includes(value.unit as TemperatureUnit)
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

  static encodeConfig(config: Aht10ConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      i2cAddress: config.i2cAddress,
      unit: config.unit,
      pollMs: config.pollMs,
      reportDeltaCelsius: config.reportDeltaCelsius,
      reportDeltaHumidity: config.reportDeltaHumidity,
      reportAlways: config.reportAlways,
      temperatureFilter: { ...config.temperatureFilter },
      humidityFilter: { ...config.humidityFilter },
    }
  }

  createDefaultConfig(): Aht10ConfigDraft {
    return Aht10Device.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Aht10CreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): Aht10CreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Aht10ConfigDraft {
    return Aht10Device.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): Aht10SensorOutputSnapshot {
    return record.runtime as Aht10SensorOutputSnapshot
  }

  protected override encodeConfig(config: Aht10ConfigDraft): Record<string, unknown> {
    return Aht10Device.encodeConfig(config)
  }

  protected override createCreateDeps(config: Aht10ConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'i2c_bus',
        deviceId: config.dependencyDeviceId,
      },
    ]
  }
}
