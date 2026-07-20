import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import { TemperatureSensorDevice } from './temperature-sensor-device.ts'
import { defaultSensorFilterConfig, normalizeSensorFilterConfig, type SensorFilterConfig } from './sensor-filter.ts'
import type {
  BaseDeviceConfig,
  DeviceDependencyLink,
  DeviceRecord,
  NtcThermistorTemperatureSensorOutputSnapshot,
  TemperatureUnit,
} from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'

export type NtcFormulaMode = 'beta' | 'steinhart_hart'

// A pure resistance->temperature calculator over an AnalogInput-role dependency (see
// docs/analog-input.md) -- it owns no ADC hardware itself, only the divider geometry
// (seriesResistorOhms/supplyMilliVolts) and the Beta/Steinhart-Hart curve.
export interface NtcThermistorConfigDraft extends BaseDeviceConfig {
  dependencyDeviceId: number
  formulaMode: NtcFormulaMode
  seriesResistorOhms: number
  supplyMilliVolts: number
  nominalResistanceOhms: number
  nominalTempCelsius: number
  betaCoefficient: number
  steinhartA: number
  steinhartB: number
  steinhartC: number
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

function normalizeDependencyDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

export class NtcThermistorDevice extends TemperatureSensorDevice<
  NtcThermistorConfigDraft,
  NtcThermistorCreateDraft,
  NtcThermistorTemperatureSensorOutputSnapshot
> {
  static readonly TYPE_ID = 10 as const
  static readonly TYPE_NAME = 'ntc_thermistor_temperature_sensor' as const
  static readonly formulaModeOptions: NtcFormulaMode[] = ['beta', 'steinhart_hart']

  readonly typeName = NtcThermistorDevice.TYPE_NAME
  readonly typeId = NtcThermistorDevice.TYPE_ID
  // Provides temperature_sensor (unchanged); additionally consumes analog_input, unlike the
  // shared TemperatureSensorDevice base which only declares the provided role.
  readonly dependencyRoles: DeviceRole[] = ['temperature_sensor']

  static defaultConfig(): NtcThermistorConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      dependencyDeviceId: 0,
      formulaMode: 'beta',
      seriesResistorOhms: 10000,
      supplyMilliVolts: 3300,
      nominalResistanceOhms: 10000,
      nominalTempCelsius: 25,
      betaCoefficient: 3950,
      steinhartA: 0,
      steinhartB: 0,
      steinhartC: 0,
      unit: 'celsius',
      pollMs: 5000,
      reportDeltaCelsius: 0.1,
      reportAlways: false,
      filter: defaultSensorFilterConfig(),
    }
  }

  static normalizeConfig(value: unknown, dependencyDeviceOrDeps?: number | DeviceDependencyLink[]): NtcThermistorConfigDraft {
    const defaults = NtcThermistorDevice.defaultConfig()
    const dependencyDeviceId = Array.isArray(dependencyDeviceOrDeps)
      ? (dependencyDeviceOrDeps.find(dep => dep.role === 'analog_input')?.deviceId ?? 0)
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
      formulaMode: NtcThermistorDevice.formulaModeOptions.includes(value.formulaMode as NtcFormulaMode)
        ? (value.formulaMode as NtcFormulaMode)
        : defaults.formulaMode,
      seriesResistorOhms: typeof value.seriesResistorOhms === 'number' && Number.isFinite(value.seriesResistorOhms)
        ? value.seriesResistorOhms
        : defaults.seriesResistorOhms,
      supplyMilliVolts: typeof value.supplyMilliVolts === 'number' && Number.isFinite(value.supplyMilliVolts)
        ? value.supplyMilliVolts
        : defaults.supplyMilliVolts,
      nominalResistanceOhms: typeof value.nominalResistanceOhms === 'number' && Number.isFinite(value.nominalResistanceOhms)
        ? value.nominalResistanceOhms
        : defaults.nominalResistanceOhms,
      nominalTempCelsius: typeof value.nominalTempCelsius === 'number' && Number.isFinite(value.nominalTempCelsius)
        ? value.nominalTempCelsius
        : defaults.nominalTempCelsius,
      betaCoefficient: typeof value.betaCoefficient === 'number' && Number.isFinite(value.betaCoefficient)
        ? value.betaCoefficient
        : defaults.betaCoefficient,
      steinhartA: typeof value.steinhartA === 'number' && Number.isFinite(value.steinhartA) ? value.steinhartA : defaults.steinhartA,
      steinhartB: typeof value.steinhartB === 'number' && Number.isFinite(value.steinhartB) ? value.steinhartB : defaults.steinhartB,
      steinhartC: typeof value.steinhartC === 'number' && Number.isFinite(value.steinhartC) ? value.steinhartC : defaults.steinhartC,
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
      formulaMode: config.formulaMode,
      seriesResistorOhms: config.seriesResistorOhms,
      supplyMilliVolts: config.supplyMilliVolts,
      nominalResistanceOhms: config.nominalResistanceOhms,
      nominalTempCelsius: config.nominalTempCelsius,
      betaCoefficient: config.betaCoefficient,
      steinhartA: config.steinhartA,
      steinhartB: config.steinhartB,
      steinhartC: config.steinhartC,
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
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): NtcThermistorConfigDraft {
    return NtcThermistorDevice.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): NtcThermistorTemperatureSensorOutputSnapshot {
    return record.runtime as NtcThermistorTemperatureSensorOutputSnapshot
  }

  protected override encodeConfig(config: NtcThermistorConfigDraft): Record<string, unknown> {
    return NtcThermistorDevice.encodeConfig(config)
  }

  protected override createCreateDeps(config: NtcThermistorConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'analog_input',
        deviceId: config.dependencyDeviceId,
      },
    ]
  }
}
