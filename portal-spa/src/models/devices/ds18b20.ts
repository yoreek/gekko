import type { OneWireScanDeviceSnapshot, TemperatureOutputSnapshot, TemperatureUnit } from '@/api/contracts'

export type Ds18b20Resolution = 9 | 10 | 11 | 12

export interface Ds18b20TemperatureSensorConfigDraft {
  enabled: boolean
  parent_device_id: number
  address: string
  resolution: Ds18b20Resolution
  unit: TemperatureUnit
  poll_ms: number
  report_delta_celsius: number
  report_always: boolean
}

export const ds18b20ResolutionOptions: Ds18b20Resolution[] = [9, 10, 11, 12]
export const temperatureUnitOptions: TemperatureUnit[] = ['celsius', 'fahrenheit']

export function createDefaultDs18b20TemperatureSensorConfig(): Ds18b20TemperatureSensorConfigDraft {
  return {
    enabled: true,
    parent_device_id: 0,
    address: '',
    resolution: 12,
    unit: 'celsius',
    poll_ms: 5000,
    report_delta_celsius: 0.01,
    report_always: false,
  }
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeResolution(value: unknown): Ds18b20Resolution {
  return ds18b20ResolutionOptions.includes(value as Ds18b20Resolution) ? value as Ds18b20Resolution : 12
}

function normalizeUnit(value: unknown): TemperatureUnit {
  return temperatureUnitOptions.includes(value as TemperatureUnit) ? value as TemperatureUnit : 'celsius'
}

function normalizeParentDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

export function normalizeDs18b20TemperatureSensorConfig(
  value: unknown,
  parentDeviceId?: number,
): Ds18b20TemperatureSensorConfigDraft {
  const defaults = createDefaultDs18b20TemperatureSensorConfig()
  if (!isRecord(value)) {
    return {
      ...defaults,
      parent_device_id: normalizeParentDeviceId(parentDeviceId),
    }
  }
  const reportDeltaCenti = typeof value.report_delta_centi_celsius === 'number' ? value.report_delta_centi_celsius / 100 : undefined
  const normalizedParentDeviceId = normalizeParentDeviceId(parentDeviceId ?? value.parent_device_id)
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : defaults.enabled,
    parent_device_id: normalizedParentDeviceId,
    address: typeof value.address === 'string' ? value.address.toUpperCase() : defaults.address,
    resolution: normalizeResolution(value.resolution),
    unit: normalizeUnit(value.unit),
    poll_ms: typeof value.poll_ms === 'number' && Number.isFinite(value.poll_ms) ? value.poll_ms : defaults.poll_ms,
    report_delta_celsius: typeof value.report_delta_celsius === 'number' && Number.isFinite(value.report_delta_celsius)
      ? value.report_delta_celsius
      : reportDeltaCenti ?? defaults.report_delta_celsius,
    report_always: typeof value.report_always === 'boolean' ? value.report_always : defaults.report_always,
  }
}

export function ds18b20ConfigChanged(
  current: Ds18b20TemperatureSensorConfigDraft,
  original: Ds18b20TemperatureSensorConfigDraft,
): boolean {
  return (
    current.parent_device_id !== original.parent_device_id ||
    current.address.toUpperCase() !== original.address.toUpperCase() ||
    current.resolution !== original.resolution ||
    current.unit !== original.unit ||
    current.poll_ms !== original.poll_ms ||
    current.report_delta_celsius !== original.report_delta_celsius ||
    current.report_always !== original.report_always ||
    current.enabled !== original.enabled
  )
}

export function encodeDs18b20Config(config: Ds18b20TemperatureSensorConfigDraft): Record<string, unknown> {
  return {
    enabled: config.enabled,
    address: config.address.trim().toUpperCase(),
    resolution: config.resolution,
    unit: config.unit,
    poll_ms: config.poll_ms,
    report_delta_celsius: config.report_delta_celsius,
    report_always: config.report_always,
  }
}

export function ds18b20AddressShapeValid(address: string): boolean {
  return /^[0-9A-Fa-f]{16}$/.test(address.trim())
}

export function isDs18b20ScanCandidate(candidate: OneWireScanDeviceSnapshot): boolean {
  return candidate.family_code.toUpperCase() === '28' && ds18b20AddressShapeValid(candidate.address)
}

export function temperatureOutputValid(value: unknown): value is TemperatureOutputSnapshot {
  return isRecord(value) && typeof value.valid === 'boolean'
}

export function formatTemperatureOutput(output: TemperatureOutputSnapshot | undefined): string {
  if (!output?.valid) {
    return ''
  }
  return `${output.value.toFixed(2)} ${output.unit_symbol}`
}
