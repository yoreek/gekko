import type {
  BaseDeviceConfig,
  DeviceDependencyLink,
  TemperatureOutputSnapshot,
  ThermostatOutputSnapshot,
} from '@/api/contracts'
import { isOutputState } from '@/models/devices/switch'
import { Ds18b20 } from '@/models/devices/ds18b20'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import { THERMOSTAT_DEVICE_TYPE_ID } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'

export namespace Thermostat {
  export type Mode = 'off' | 'heat' | 'cool'
  export type Algorithm = 'hysteresis'

  export interface ConfigDraft extends BaseDeviceConfig {
    mode: Mode
    algorithm: Algorithm
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

  export interface CreateDraft extends DeviceCreateDraftBase, ConfigDraft {
    typeId: number
  }

  const modeOptions: Mode[] = ['off', 'heat', 'cool']
  const algorithmOptions: Algorithm[] = ['hysteresis']

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

  function normalizeMode(value: unknown): Mode {
    return modeOptions.includes(value as Mode) ? (value as Mode) : 'off'
  }

  function normalizeAlgorithm(value: unknown): Algorithm {
    return algorithmOptions.includes(value as Algorithm) ? (value as Algorithm) : 'hysteresis'
  }

  function deviceIdFromDeps(deps: DeviceDependencyLink[] | undefined, role: string): number {
    return deps?.find(dep => dep.role === role)?.deviceId ?? 0
  }

  export function defaultConfig(): ConfigDraft {
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

  export function normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): ConfigDraft {
    const defaults = defaultConfig()
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

  export function configChanged(current: ConfigDraft, original: ConfigDraft): boolean {
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

  export function encodeConfig(config: ConfigDraft): Record<string, unknown> {
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

  export function dependencyLinks(config: ConfigDraft): DeviceDependencyLink[] {
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

  export function modeLabelKey(mode: Mode): string {
    return `device.dialog.thermostat.mode.${mode}`
  }

  export function algorithmLabelKey(algorithm: Algorithm): string {
    return `device.dialog.thermostat.algorithm.${algorithm}`
  }

  export function statusLabelKey(status: string | undefined | null): string {
    return `device.dialog.thermostat.status.${status ?? 'unknown'}`
  }

  export function formatTemperature(value: number): string {
    return `${value.toFixed(1)}°C`
  }

  export function formatOutput(output: TemperatureOutputSnapshot | undefined): string {
    if (!output?.valid) {
      return ''
    }
    return formatTemperature(output.value)
  }

  export function normalizeOutput(value: unknown): ThermostatOutputSnapshot {
    if (!isRecord(value)) {
      return {}
    }
    return {
      desiredSwitchState: isOutputState(value.desiredSwitchState) ? value.desiredSwitchState : undefined,
      actualSwitchState: isOutputState(value.actualSwitchState) ? value.actualSwitchState : undefined,
      controlStatus: typeof value.controlStatus === 'string' ? value.controlStatus : undefined,
      lastCheckAtMs: typeof value.lastCheckAtMs === 'number' && Number.isFinite(value.lastCheckAtMs)
        ? value.lastCheckAtMs
        : undefined,
      temperature: Ds18b20.temperatureValid(value.temperature) ? value.temperature : undefined,
    }
  }

  export function outputTone(status: string | undefined | null): 'primary' | 'secondary' | 'warning' | 'error' {
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

  export function summaryText(mode: Mode, targetCelsius: number, status: string | undefined | null): string {
    const modeText = mode === 'off' ? 'Off' : mode === 'heat' ? 'Heat' : 'Cool'
    const targetText = formatTemperature(targetCelsius)
    const statusText = status && status !== 'ready' && status !== 'ok' ? ` · ${status}` : ''
    return `${modeText} · ${targetText}${statusText}`
  }

  export class Device extends BaseDevice<ConfigDraft, CreateDraft, ThermostatOutputSnapshot> {
    readonly typeName = 'thermostat'
    readonly typeId = THERMOSTAT_DEVICE_TYPE_ID

    createDefaultConfig(): ConfigDraft {
      return Thermostat.defaultConfig()
    }

    createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): CreateDraft {
      return {
        ...this.createDefaultConfig(),
        ...common,
        typeId: common.typeId ?? this.typeId,
      }
    }

    createEditDraft(current: DeviceRecord): CreateDraft {
      return {
        ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
        typeId: this.typeId,
      }
    }

    normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): ConfigDraft {
      return Thermostat.normalizeConfig(value, deps)
    }

    normalizeOutput(record: DeviceRecord): ThermostatOutputSnapshot {
      return Thermostat.normalizeOutput(record.runtime)
    }

    override encodeConfig(config: ConfigDraft): Record<string, unknown> {
      return Thermostat.encodeConfig(config)
    }

    buildEditCommands(current: DeviceRecord, draft: CreateDraft): DeviceCommandRequest[] {
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
      if (Thermostat.configChanged(nextConfig, currentConfig)) {
        commands.push({
          command: 'updateConfig',
          config: {
            ...this.encodeConfig(nextConfig),
            enabled: draft.enabled,
          },
          deps: Thermostat.dependencyLinks(nextConfig),
        })
      }
      return commands
    }

    protected extractCreateConfig(draft: CreateDraft): ConfigDraft {
      const { typeId: _typeId, ...config } = draft
      return config
    }

    protected override createCreateDeps(config: ConfigDraft) {
      return Thermostat.dependencyLinks(config)
    }
  }
}
