import type {
  DeviceDependencyLink,
  TemperatureOutputSnapshot,
  ThermostatOutputSnapshot,
} from '@/api/contracts'
import { isOutputState } from '@/models/devices/switch'
import { Ds18b20 } from '@/models/devices/ds18b20'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import type { DashboardDevice } from '@/models/device'
import { THERMOSTAT_DEVICE_TYPE_ID } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'

export namespace Thermostat {
  export type Mode = 'off' | 'heat' | 'cool'
  export type Algorithm = 'hysteresis'

  export interface ConfigDraft {
    enabled: boolean
    mode: Mode
    algorithm: Algorithm
    target_celsius: number
    min_safe_celsius: number
    max_safe_celsius: number
    hysteresis_celsius: number
    check_interval_ms: number
    sensor_timeout_ms: number
    retry_after_error_ms: number
    min_switch_interval_ms: number
    temperature_sensor_device_id: number
    switch_device_id: number
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
    return deps?.find(dep => dep.role === role)?.device_id ?? 0
  }

  export function defaultConfig(): ConfigDraft {
    return {
      enabled: true,
      mode: 'heat',
      algorithm: 'hysteresis',
      target_celsius: 25,
      min_safe_celsius: 0,
      max_safe_celsius: 50,
      hysteresis_celsius: 0.5,
      check_interval_ms: 1000,
      sensor_timeout_ms: 6000,
      retry_after_error_ms: 30000,
      min_switch_interval_ms: 5000,
      temperature_sensor_device_id: 0,
      switch_device_id: 0,
    }
  }

  export function normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): ConfigDraft {
    const defaults = defaultConfig()
    if (!isRecord(value)) {
      return {
        ...defaults,
        temperature_sensor_device_id: deviceIdFromDeps(deps, 'temperature_sensor'),
        switch_device_id: deviceIdFromDeps(deps, 'switch'),
      }
    }

    return {
      enabled: typeof value.enabled === 'boolean' ? value.enabled : defaults.enabled,
      mode: normalizeMode(value.mode),
      algorithm: normalizeAlgorithm(value.algorithm),
      target_celsius: normalizeNumber(value.target_milli_celsius, defaults.target_celsius * 1000) / 1000,
      min_safe_celsius: normalizeNumber(value.min_safe_milli_celsius, defaults.min_safe_celsius * 1000) / 1000,
      max_safe_celsius: normalizeNumber(value.max_safe_milli_celsius, defaults.max_safe_celsius * 1000) / 1000,
      hysteresis_celsius: normalizeNumber(value.hysteresis_centi_celsius, defaults.hysteresis_celsius * 100) / 100,
      check_interval_ms: normalizeNumber(value.check_interval_ms, defaults.check_interval_ms),
      sensor_timeout_ms: normalizeNumber(value.sensor_timeout_ms, defaults.sensor_timeout_ms),
      retry_after_error_ms: normalizeNumber(value.retry_after_error_ms, defaults.retry_after_error_ms),
      min_switch_interval_ms: normalizeNumber(value.min_switch_interval_ms, defaults.min_switch_interval_ms),
      temperature_sensor_device_id: normalizeDeviceId(value.temperature_sensor_device_id ?? deviceIdFromDeps(deps, 'temperature_sensor')),
      switch_device_id: normalizeDeviceId(value.switch_device_id ?? deviceIdFromDeps(deps, 'switch')),
    }
  }

  export function configChanged(current: ConfigDraft, original: ConfigDraft): boolean {
    return (
      current.enabled !== original.enabled ||
      current.mode !== original.mode ||
      current.algorithm !== original.algorithm ||
      current.target_celsius !== original.target_celsius ||
      current.min_safe_celsius !== original.min_safe_celsius ||
      current.max_safe_celsius !== original.max_safe_celsius ||
      current.hysteresis_celsius !== original.hysteresis_celsius ||
      current.check_interval_ms !== original.check_interval_ms ||
      current.sensor_timeout_ms !== original.sensor_timeout_ms ||
      current.retry_after_error_ms !== original.retry_after_error_ms ||
      current.min_switch_interval_ms !== original.min_switch_interval_ms ||
      current.temperature_sensor_device_id !== original.temperature_sensor_device_id ||
      current.switch_device_id !== original.switch_device_id
    )
  }

  export function encodeConfig(config: ConfigDraft): Record<string, unknown> {
    return {
      enabled: config.enabled,
      mode: config.mode,
      algorithm: config.algorithm,
      target_milli_celsius: Math.round(config.target_celsius * 1000),
      min_safe_milli_celsius: Math.round(config.min_safe_celsius * 1000),
      max_safe_milli_celsius: Math.round(config.max_safe_celsius * 1000),
      hysteresis_centi_celsius: Math.round(config.hysteresis_celsius * 100),
      check_interval_ms: Math.round(config.check_interval_ms),
      sensor_timeout_ms: Math.round(config.sensor_timeout_ms),
      retry_after_error_ms: Math.round(config.retry_after_error_ms),
      min_switch_interval_ms: Math.round(config.min_switch_interval_ms),
    }
  }

  export function dependencyLinks(config: ConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'temperature_sensor',
        device_id: config.temperature_sensor_device_id,
      },
      {
        role: 'switch',
        device_id: config.switch_device_id,
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
      desired_switch_state: isOutputState(value.desired_switch_state) ? value.desired_switch_state : undefined,
      actual_switch_state: isOutputState(value.actual_switch_state) ? value.actual_switch_state : undefined,
      control_status: typeof value.control_status === 'string' ? value.control_status : undefined,
      last_check_at_ms: typeof value.last_check_at_ms === 'number' && Number.isFinite(value.last_check_at_ms)
        ? value.last_check_at_ms
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
        name: common.name ?? 'New Device',
        typeId: common.typeId ?? this.typeId,
        ...this.createDefaultConfig(),
        enabled: common.enabled ?? true,
      }
    }

    createEditDraft(current: DashboardDevice): CreateDraft {
      const { enabled: _enabled, ...config } = this.normalizeConfig(current.detail.config, current.deps)
      return {
        name: current.name,
        typeId: current.typeId,
        enabled: current.enabled,
        ...config,
      }
    }

    normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): ConfigDraft {
      return Thermostat.normalizeConfig(value, deps)
    }

    normalizeOutput(record: DeviceRecord): ThermostatOutputSnapshot {
      return Thermostat.normalizeOutput(record.output)
    }

    override encodeConfig(config: ConfigDraft): Record<string, unknown> {
      return Thermostat.encodeConfig(config)
    }

    buildEditCommands(current: DashboardDevice, draft: CreateDraft): DeviceCommandRequest[] {
      const commands: DeviceCommandRequest[] = []
      if (draft.name.trim() !== current.name) {
        commands.push({ command: 'rename', name: draft.name.trim() })
      }
      if (draft.enabled !== current.enabled) {
        commands.push({ command: draft.enabled ? 'enable' : 'disable' })
      }
      const currentConfig = this.normalizeConfig(current.detail.config, current.deps)
      const nextConfig = this.normalizeConfig(draft, [
        {
          role: 'temperature_sensor',
          device_id: draft.temperature_sensor_device_id,
        },
        {
          role: 'switch',
          device_id: draft.switch_device_id,
        },
      ])
      if (Thermostat.configChanged(nextConfig, currentConfig)) {
        commands.push({
          command: 'update_config',
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
      const { name: _name, typeId: _typeId, ...config } = draft
      return config
    }

    protected override createCreateDeps(config: ConfigDraft) {
      return Thermostat.dependencyLinks(config)
    }
  }
}
