import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import { TemperatureSensorDevice } from './temperature-sensor-device.ts'
import { defaultSensorFilterConfig, normalizeSensorFilterConfig, type SensorFilterConfig } from './sensor-filter.ts'
import type {
  BaseDeviceConfig,
  DeviceDependencyLink,
  DeviceRecord,
  Ds18b20TemperatureSensorOutputSnapshot,
  OneWireScanDeviceSnapshot,
  TemperatureUnit,
} from '@/api/contracts'

export type Ds18b20Resolution = 9 | 10 | 11 | 12

export interface Ds18b20ConfigDraft extends BaseDeviceConfig {
  dependencyDeviceId: number
  address: string
  resolution: Ds18b20Resolution
  unit: TemperatureUnit
  pollMs: number
  reportDeltaCelsius: number
  reportAlways: boolean
  filter: SensorFilterConfig
}

export interface Ds18b20CreateDraft extends DeviceCreateDraftBase, Ds18b20ConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeDependencyDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

export class Ds18b20Device extends TemperatureSensorDevice<
  Ds18b20ConfigDraft,
  Ds18b20CreateDraft,
  Ds18b20TemperatureSensorOutputSnapshot
> {
  static readonly TYPE_ID = 4 as const
  static readonly TYPE_NAME = 'ds18b20_temperature_sensor' as const
  static readonly resolutionOptions: Ds18b20Resolution[] = [9, 10, 11, 12]

  readonly typeName = Ds18b20Device.TYPE_NAME
  readonly typeId = Ds18b20Device.TYPE_ID

  static defaultConfig(): Ds18b20ConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      dependencyDeviceId: 0,
      address: '',
      resolution: 12,
      unit: 'celsius',
      pollMs: 5000,
      reportDeltaCelsius: 0.01,
      reportAlways: false,
      filter: defaultSensorFilterConfig(),
    }
  }

  static normalizeConfig(
    value: unknown,
    dependencyDeviceOrDeps?: number | DeviceDependencyLink[],
  ): Ds18b20ConfigDraft {
    const defaults = Ds18b20Device.defaultConfig()
    const dependencyDeviceId = Array.isArray(dependencyDeviceOrDeps)
      ? dependencyDeviceOrDeps.find(dep => dep.role === 'onewire_bus')?.deviceId ?? 0
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
    const normalizedDependencyDeviceId = normalizeDependencyDeviceId(
      dependencyDeviceId ?? value.dependencyDeviceId,
    )
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      dependencyDeviceId: normalizedDependencyDeviceId,
      address: typeof value.address === 'string' ? value.address.toUpperCase() : defaults.address,
      resolution: Ds18b20Device.resolutionOptions.includes(value.resolution as Ds18b20Resolution) ? (value.resolution as Ds18b20Resolution) : 12,
      unit: Ds18b20Device.temperatureUnitOptions.includes(value.unit as TemperatureUnit) ? (value.unit as TemperatureUnit) : 'celsius',
      pollMs: typeof value.pollMs === 'number' && Number.isFinite(value.pollMs) ? value.pollMs : defaults.pollMs,
      reportDeltaCelsius: typeof value.reportDeltaCelsius === 'number' && Number.isFinite(value.reportDeltaCelsius)
        ? value.reportDeltaCelsius
        : reportDeltaCenti ?? defaults.reportDeltaCelsius,
      reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : defaults.reportAlways,
      filter: normalizeSensorFilterConfig(value, defaults.filter),
    }
  }

  static encodeConfig(config: Ds18b20ConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      address: config.address.trim().toUpperCase(),
      resolution: config.resolution,
      unit: config.unit,
      pollMs: config.pollMs,
      reportDeltaCelsius: config.reportDeltaCelsius,
      reportAlways: config.reportAlways,
      smoothingWeight: config.filter.smoothingWeight,
      calibrationFactor: config.filter.calibrationFactor,
      calibrationOffset: config.filter.calibrationOffset,
    }
  }

  static addressValid(address: string): boolean {
    return /^[0-9A-Fa-f]{16}$/.test(address.trim())
  }

  static isScanCandidate(candidate: OneWireScanDeviceSnapshot): boolean {
    return candidate.familyCode.toUpperCase() === '28' && Ds18b20Device.addressValid(candidate.address)
  }

  createDefaultConfig(): Ds18b20ConfigDraft {
    return Ds18b20Device.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Ds18b20CreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): Ds18b20CreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Ds18b20ConfigDraft {
    return Ds18b20Device.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): Ds18b20TemperatureSensorOutputSnapshot {
    return record.runtime as Ds18b20TemperatureSensorOutputSnapshot
  }

  protected override encodeConfig(config: Ds18b20ConfigDraft): Record<string, unknown> {
    return Ds18b20Device.encodeConfig(config)
  }

  protected override createCreateDeps(config: Ds18b20ConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'onewire_bus',
        deviceId: config.dependencyDeviceId,
      },
    ]
  }
}
