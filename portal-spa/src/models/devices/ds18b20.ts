import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'
import type {
  DeviceCommandRequest,
  DeviceDependencyLink,
  DeviceRecord,
  Ds18b20TemperatureSensorOutputSnapshot,
  OneWireScanDeviceSnapshot,
  TemperatureOutputSnapshot,
  TemperatureUnit,
} from '@/api/contracts'
import type { DashboardDevice } from '@/models/device'

export namespace Ds18b20 {
  export type Resolution = 9 | 10 | 11 | 12

  export interface ConfigDraft {
    enabled: boolean
    dependency_device_id: number
    address: string
    resolution: Resolution
    unit: TemperatureUnit
    poll_ms: number
    report_delta_celsius: number
    report_always: boolean
  }

  export interface CreateDraft extends DeviceCreateDraftBase, ConfigDraft {
    typeId: number
  }

  export const resolutionOptions: Resolution[] = [9, 10, 11, 12]
  export const temperatureUnitOptions: TemperatureUnit[] = ['celsius', 'fahrenheit']

  function isRecord(value: unknown): value is Record<string, unknown> {
    return typeof value === 'object' && value !== null && !Array.isArray(value)
  }

  function normalizeResolution(value: unknown): Resolution {
    return resolutionOptions.includes(value as Resolution) ? (value as Resolution) : 12
  }

  function normalizeUnit(value: unknown): TemperatureUnit {
    return temperatureUnitOptions.includes(value as TemperatureUnit) ? (value as TemperatureUnit) : 'celsius'
  }

  function normalizeDependencyDeviceId(value: unknown): number {
    const numeric = Number(value)
    return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
  }

  function createDefaultConfig(): ConfigDraft {
    return {
      enabled: true,
      dependency_device_id: 0,
      address: '',
      resolution: 12,
      unit: 'celsius',
      poll_ms: 5000,
      report_delta_celsius: 0.01,
      report_always: false,
    }
  }

  export function defaultConfig(): ConfigDraft {
    return createDefaultConfig()
  }

  export function normalizeConfig(
    value: unknown,
    dependencyDeviceOrDeps?: number | DeviceDependencyLink[],
  ): ConfigDraft {
    const defaults = createDefaultConfig()
    const dependencyDeviceId = Array.isArray(dependencyDeviceOrDeps)
      ? dependencyDeviceOrDeps.find(dep => dep.role === 'onewire_bus')?.device_id ?? 0
      : typeof dependencyDeviceOrDeps === 'number'
        ? dependencyDeviceOrDeps
        : 0
    if (!isRecord(value)) {
      return {
        ...defaults,
        dependency_device_id: normalizeDependencyDeviceId(dependencyDeviceId),
      }
    }
    const reportDeltaCenti = typeof value.report_delta_centi_celsius === 'number' ? value.report_delta_centi_celsius / 100 : undefined
    const normalizedDependencyDeviceId = normalizeDependencyDeviceId(
      dependencyDeviceId ?? value.dependency_device_id,
    )
    return {
      enabled: typeof value.enabled === 'boolean' ? value.enabled : defaults.enabled,
      dependency_device_id: normalizedDependencyDeviceId,
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

  export function configChanged(
    current: ConfigDraft,
    original: ConfigDraft,
  ): boolean {
    return (
      current.dependency_device_id !== original.dependency_device_id ||
      current.address.toUpperCase() !== original.address.toUpperCase() ||
      current.resolution !== original.resolution ||
      current.unit !== original.unit ||
      current.poll_ms !== original.poll_ms ||
      current.report_delta_celsius !== original.report_delta_celsius ||
      current.report_always !== original.report_always ||
      current.enabled !== original.enabled
    )
  }

  export function encodeConfig(config: ConfigDraft): Record<string, unknown> {
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

  export function addressValid(address: string): boolean {
    return /^[0-9A-Fa-f]{16}$/.test(address.trim())
  }

  export function isScanCandidate(candidate: OneWireScanDeviceSnapshot): boolean {
    return candidate.family_code.toUpperCase() === '28' && addressValid(candidate.address)
  }

  export function temperatureValid(value: unknown): value is TemperatureOutputSnapshot {
    return isRecord(value) && typeof value.valid === 'boolean'
  }

  export function normalizeOutput(value: unknown): Ds18b20TemperatureSensorOutputSnapshot {
    if (!isRecord(value)) {
      return {}
    }
    return {
      temperature: temperatureValid(value.temperature) ? value.temperature : undefined,
    }
  }

  export function formatTemperature(output: TemperatureOutputSnapshot | undefined): string {
    if (!output?.valid) {
      return ''
    }
    return `${output.value.toFixed(2)} ${output.unit_symbol}`
  }

  export class Device extends BaseDevice<
    ConfigDraft,
    CreateDraft,
    Ds18b20TemperatureSensorOutputSnapshot
  > {
    readonly typeName = 'ds18b20_temperature_sensor'
    readonly typeId = DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID

    createDefaultConfig(): ConfigDraft {
      return Ds18b20.defaultConfig()
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

    normalizeConfig(value: unknown, deps?: number | { role: string; device_id: number }[]): ConfigDraft {
      return Ds18b20.normalizeConfig(value, deps)
    }

    normalizeOutput(record: DeviceRecord): Ds18b20TemperatureSensorOutputSnapshot {
      return Ds18b20.normalizeOutput(record.output)
    }

    override encodeConfig(config: ConfigDraft): Record<string, unknown> {
      return Ds18b20.encodeConfig(config)
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
          role: 'onewire_bus',
          device_id: draft.dependency_device_id,
        },
      ])
      if (Ds18b20.configChanged(nextConfig, currentConfig)) {
        commands.push({
          command: 'update_config',
          config: {
            ...this.encodeConfig(nextConfig),
            enabled: draft.enabled,
          },
          deps: [
            {
              role: 'onewire_bus',
              device_id: nextConfig.dependency_device_id,
            },
          ],
        })
      }
      return commands
    }

    protected extractCreateConfig(draft: CreateDraft): ConfigDraft {
      const { name: _name, typeId: _typeId, ...config } = draft
      return config
    }

    protected override createCreateDeps(config: ConfigDraft) {
      return [
        {
          role: 'onewire_bus',
          device_id: config.dependency_device_id,
        },
      ]
    }
  }
}
