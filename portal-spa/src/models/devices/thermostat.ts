import type {
  BaseDeviceConfig,
  DeviceDependencyLink,
  TemperatureOutputSnapshot,
  ThermostatOutputSnapshot,
} from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import { BaseDevice } from './base-device.ts'

export type ThermostatMode = 'off' | 'heat' | 'cool'
export type ThermostatAlgorithm = 'hysteresis'

export interface ThermostatConfigDraft extends BaseDeviceConfig {
  mode: ThermostatMode
  algorithm: ThermostatAlgorithm
  targetCelsius: number
  minSafeCelsius: number
  maxSafeCelsius: number
  hysteresisCelsius: number
  checkIntervalMs: number
  sensorTimeoutMs: number
  retryAfterErrorMs: number
  minSwitchIntervalMs: number
  temperatureSensorDeviceId: number
  switchDeviceId: number
}

export interface ThermostatCreateDraft extends DeviceCreateDraftBase, ThermostatConfigDraft {}

const modeOptions: ThermostatMode[] = ['off', 'heat', 'cool']
const algorithmOptions: ThermostatAlgorithm[] = ['hysteresis']

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeNumber(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) ? numeric : fallback
}

function normalizeDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

function normalizeMode(value: unknown): ThermostatMode {
  return modeOptions.includes(value as ThermostatMode) ? (value as ThermostatMode) : 'off'
}

function normalizeAlgorithm(value: unknown): ThermostatAlgorithm {
  return algorithmOptions.includes(value as ThermostatAlgorithm) ? (value as ThermostatAlgorithm) : 'hysteresis'
}

function deviceIdFromDeps(deps: DeviceDependencyLink[] | undefined, role: string): number {
  return deps?.find(dep => dep.role === role)?.deviceId ?? 0
}

export class ThermostatDevice extends BaseDevice<ThermostatConfigDraft, ThermostatCreateDraft, ThermostatOutputSnapshot> {
  static readonly TYPE_ID = 5 as const
  static readonly TYPE_NAME = 'thermostat' as const

  readonly typeName = ThermostatDevice.TYPE_NAME
  readonly typeId = ThermostatDevice.TYPE_ID

  static defaultConfig(): ThermostatConfigDraft {
    return {
      enabled: true,
      name: 'New Device',
      deps: [],
      mode: 'heat',
      algorithm: 'hysteresis',
      targetCelsius: 25,
      minSafeCelsius: 0,
      maxSafeCelsius: 50,
      hysteresisCelsius: 0.5,
      checkIntervalMs: 1000,
      sensorTimeoutMs: 6000,
      retryAfterErrorMs: 30000,
      minSwitchIntervalMs: 5000,
      temperatureSensorDeviceId: 0,
      switchDeviceId: 0,
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): ThermostatConfigDraft {
    const defaults = ThermostatDevice.defaultConfig()
    if (!isRecord(value)) {
      return {
        ...defaults,
        deps: Array.isArray(deps) ? deps : defaults.deps,
        temperatureSensorDeviceId: deviceIdFromDeps(deps, 'temperature_sensor'),
        switchDeviceId: deviceIdFromDeps(deps, 'switch'),
      }
    }

    return {
      name: typeof value.name === 'string' ? value.name : defaults.name,
      enabled: typeof value.enabled === 'boolean' ? value.enabled : defaults.enabled,
      deps: Array.isArray(value.deps) ? (value.deps as DeviceDependencyLink[]) : defaults.deps,
      mode: normalizeMode(value.mode),
      algorithm: normalizeAlgorithm(value.algorithm),
      targetCelsius: normalizeNumber(value.targetCelsius, defaults.targetCelsius),
      minSafeCelsius: normalizeNumber(value.minSafeCelsius, defaults.minSafeCelsius),
      maxSafeCelsius: normalizeNumber(value.maxSafeCelsius, defaults.maxSafeCelsius),
      hysteresisCelsius: normalizeNumber(value.hysteresisCelsius, defaults.hysteresisCelsius),
      checkIntervalMs: normalizeNumber(value.checkIntervalMs, defaults.checkIntervalMs),
      sensorTimeoutMs: normalizeNumber(value.sensorTimeoutMs, defaults.sensorTimeoutMs),
      retryAfterErrorMs: normalizeNumber(value.retryAfterErrorMs, defaults.retryAfterErrorMs),
      minSwitchIntervalMs: normalizeNumber(value.minSwitchIntervalMs, defaults.minSwitchIntervalMs),
      temperatureSensorDeviceId: normalizeDeviceId(value.temperatureSensorDeviceId ?? deviceIdFromDeps(deps, 'temperature_sensor')),
      switchDeviceId: normalizeDeviceId(value.switchDeviceId ?? deviceIdFromDeps(deps, 'switch')),
    }
  }

  static configChanged(current: ThermostatConfigDraft, original: ThermostatConfigDraft): boolean {
    return (
      current.enabled !== original.enabled ||
      current.mode !== original.mode ||
      current.algorithm !== original.algorithm ||
      current.targetCelsius !== original.targetCelsius ||
      current.minSafeCelsius !== original.minSafeCelsius ||
      current.maxSafeCelsius !== original.maxSafeCelsius ||
      current.hysteresisCelsius !== original.hysteresisCelsius ||
      current.checkIntervalMs !== original.checkIntervalMs ||
      current.sensorTimeoutMs !== original.sensorTimeoutMs ||
      current.retryAfterErrorMs !== original.retryAfterErrorMs ||
      current.minSwitchIntervalMs !== original.minSwitchIntervalMs ||
      current.temperatureSensorDeviceId !== original.temperatureSensorDeviceId ||
      current.switchDeviceId !== original.switchDeviceId
    )
  }

  static encodeConfig(config: ThermostatConfigDraft): Record<string, unknown> {
    return {
      name: config.name,
      enabled: config.enabled,
      deps: config.deps,
      mode: config.mode,
      algorithm: config.algorithm,
      targetCelsius: config.targetCelsius,
      minSafeCelsius: config.minSafeCelsius,
      maxSafeCelsius: config.maxSafeCelsius,
      hysteresisCelsius: config.hysteresisCelsius,
      checkIntervalMs: Math.round(config.checkIntervalMs),
      sensorTimeoutMs: Math.round(config.sensorTimeoutMs),
      retryAfterErrorMs: Math.round(config.retryAfterErrorMs),
      minSwitchIntervalMs: Math.round(config.minSwitchIntervalMs),
    }
  }

  static dependencyLinks(config: ThermostatConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'temperature_sensor',
        deviceId: config.temperatureSensorDeviceId,
      },
      {
        role: 'switch',
        deviceId: config.switchDeviceId,
      },
    ]
  }

  static modeLabelKey(mode: ThermostatMode): string {
    return `device.dialog.thermostat.mode.${mode}`
  }

  static algorithmLabelKey(algorithm: ThermostatAlgorithm): string {
    return `device.dialog.thermostat.algorithm.${algorithm}`
  }

  static statusLabelKey(status: string | undefined | null): string {
    return `device.dialog.thermostat.status.${status ?? 'unknown'}`
  }

  static formatTemperature(value: number): string {
    return `${value.toFixed(1)}°C`
  }

  static formatOutput(output: TemperatureOutputSnapshot | undefined): string {
    if (!output?.valid) {
      return ''
    }
    return ThermostatDevice.formatTemperature(output.value)
  }

  static outputTone(status: string | undefined | null): 'primary' | 'secondary' | 'warning' | 'error' {
    switch (status) {
      case 'ready':
      case 'ok':
        return 'primary'
      case 'disabled':
        return 'secondary'
      case 'dependency_blocked':
      case 'sensor_timeout':
      case 'out_of_range':
      case 'retry_backoff':
        return 'warning'
      case 'faulted':
      case 'switch_error':
        return 'error'
      default:
        return 'secondary'
    }
  }

  static summaryText(mode: ThermostatMode, targetCelsius: number, status: string | undefined | null): string {
    const modeText = mode === 'off' ? 'Off' : mode === 'heat' ? 'Heat' : 'Cool'
    const targetText = ThermostatDevice.formatTemperature(targetCelsius)
    const statusText = status && status !== 'ready' && status !== 'ok' ? ` · ${status}` : ''
    return `${modeText} · ${targetText}${statusText}`
  }

  createDefaultConfig(): ThermostatConfigDraft {
    return ThermostatDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): ThermostatCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): ThermostatCreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): ThermostatConfigDraft {
    return ThermostatDevice.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): ThermostatOutputSnapshot {
    return record.runtime as ThermostatOutputSnapshot
  }

  override encodeConfig(config: ThermostatConfigDraft): Record<string, unknown> {
    return ThermostatDevice.encodeConfig(config)
  }

  buildEditCommands(current: DeviceRecord, draft: ThermostatCreateDraft): DeviceCommandRequest[] {
    const currentConfig = this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined)
    const commands: DeviceCommandRequest[] = []
    if (draft.name.trim() !== currentConfig.name) {
      commands.push({ command: 'rename', name: draft.name.trim() })
    }
    if (draft.enabled !== currentConfig.enabled) {
      commands.push({ command: draft.enabled ? 'enable' : 'disable' })
    }
    const nextConfig = this.normalizeConfig(draft, [
      {
        role: 'temperature_sensor',
        deviceId: draft.temperatureSensorDeviceId,
      },
      {
        role: 'switch',
        deviceId: draft.switchDeviceId,
      },
    ])
    if (ThermostatDevice.configChanged(nextConfig, currentConfig)) {
      commands.push({
        command: 'updateConfig',
        config: {
          ...this.encodeConfig(nextConfig),
          enabled: draft.enabled,
        },
        deps: ThermostatDevice.dependencyLinks(nextConfig),
      })
    }
    return commands
  }

  protected extractCreateConfig(draft: ThermostatCreateDraft): ThermostatConfigDraft {
    return { ...draft }
  }

  protected override createCreateDeps(config: ThermostatConfigDraft) {
    return ThermostatDevice.dependencyLinks(config)
  }
}
