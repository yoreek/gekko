import type { DeviceDependencyLink, DeviceOutputState, TemperatureOutputSnapshot } from '@/api/contracts'

export type ThermostatMode = 'off' | 'heat' | 'cool'
export type ThermostatAlgorithm = 'hysteresis'

export interface ThermostatConfigDraft {
  enabled: boolean
  mode: ThermostatMode
  algorithm: ThermostatAlgorithm
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

const thermostatModeOptions: ThermostatMode[] = ['off', 'heat', 'cool']
const thermostatAlgorithmOptions: ThermostatAlgorithm[] = ['hysteresis']

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
  return thermostatModeOptions.includes(value as ThermostatMode) ? (value as ThermostatMode) : 'off'
}

function normalizeAlgorithm(value: unknown): ThermostatAlgorithm {
  return thermostatAlgorithmOptions.includes(value as ThermostatAlgorithm) ? (value as ThermostatAlgorithm) : 'hysteresis'
}

function deviceIdFromDeps(deps: DeviceDependencyLink[] | undefined, role: string): number {
  return deps?.find(dep => dep.role === role)?.device_id ?? 0
}

export function createDefaultThermostatConfig(): ThermostatConfigDraft {
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

export function normalizeThermostatConfig(value: unknown, deps?: DeviceDependencyLink[]): ThermostatConfigDraft {
  const defaults = createDefaultThermostatConfig()
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

export function thermostatConfigChanged(current: ThermostatConfigDraft, original: ThermostatConfigDraft): boolean {
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

export function encodeThermostatConfig(config: ThermostatConfigDraft): Record<string, unknown> {
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

export function thermostatDependencyLinks(config: ThermostatConfigDraft): DeviceDependencyLink[] {
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

export function thermostatModeLabelKey(mode: ThermostatMode): string {
  return `device.dialog.thermostatMode.${mode}`
}

export function thermostatAlgorithmLabelKey(algorithm: ThermostatAlgorithm): string {
  return `device.dialog.thermostatAlgorithm.${algorithm}`
}

export function thermostatStatusLabelKey(status: string | undefined | null): string {
  return `device.dialog.thermostatStatus.${status ?? 'unknown'}`
}

export function formatThermostatTemperature(value: number): string {
  return `${value.toFixed(1)}°C`
}

export function formatThermostatOutput(output: TemperatureOutputSnapshot | undefined): string {
  if (!output?.valid) {
    return ''
  }
  return formatThermostatTemperature(output.value)
}

export function thermostatOutputTone(status: string | undefined | null): 'primary' | 'secondary' | 'warning' | 'error' {
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

export function thermostatSummaryText(mode: ThermostatMode, targetCelsius: number, status: string | undefined | null): string {
  const modeText = mode === 'off' ? 'Off' : mode === 'heat' ? 'Heat' : 'Cool'
  const targetText = formatThermostatTemperature(targetCelsius)
  const statusText = status && status !== 'ready' && status !== 'ok' ? ` · ${status}` : ''
  return `${modeText} · ${targetText}${statusText}`
}
