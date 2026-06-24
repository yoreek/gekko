import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'
import type {
  BaseDeviceConfig,
  DeviceCommandRequest,
  DeviceDependencyLink,
  DeviceRecord,
  Ds18b20TemperatureSensorOutputSnapshot,
  OneWireScanDeviceSnapshot,
  TemperatureOutputSnapshot,
  TemperatureUnit,
} from '@/api/contracts'

export namespace Ds18b20 {
  export type Resolution = 9 | 10 | 11 | 12

  export interface ConfigDraft extends BaseDeviceConfig {
    dependencyDeviceId: number
    address: string
    resolution: Resolution
    unit: TemperatureUnit
    pollMs: number
    reportDeltaCelsius: number
    reportAlways: boolean
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
      name: 'New Device',
      deps: [],
      dependencyDeviceId: 0,
      address: '',
      resolution: 12,
      unit: 'celsius',
      pollMs: 5000,
      reportDeltaCelsius: 0.01,
      reportAlways: false,
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
    const deps = Array.isArray(value.deps) ? (value.deps as DeviceDependencyLink[]) : defaults.deps
    const normalizedDependencyDeviceId = normalizeDependencyDeviceId(
      dependencyDeviceId ?? value.dependencyDeviceId,
    )
    return {
      name: typeof value.name === 'string' ? value.name : defaults.name,
      enabled: typeof value.enabled === 'boolean' ? value.enabled : defaults.enabled,
      deps,
      dependencyDeviceId: normalizedDependencyDeviceId,
      address: typeof value.address === 'string' ? value.address.toUpperCase() : defaults.address,
      resolution: normalizeResolution(value.resolution),
      unit: normalizeUnit(value.unit),
      pollMs: typeof value.pollMs === 'number' && Number.isFinite(value.pollMs) ? value.pollMs : defaults.pollMs,
      reportDeltaCelsius: typeof value.reportDeltaCelsius === 'number' && Number.isFinite(value.reportDeltaCelsius)
        ? value.reportDeltaCelsius
        : reportDeltaCenti ?? defaults.reportDeltaCelsius,
      reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : defaults.reportAlways,
    }
  }

  export function configChanged(
    current: ConfigDraft,
    original: ConfigDraft,
  ): boolean {
    return (
      current.dependencyDeviceId !== original.dependencyDeviceId ||
      current.address.toUpperCase() !== original.address.toUpperCase() ||
      current.resolution !== original.resolution ||
      current.unit !== original.unit ||
      current.pollMs !== original.pollMs ||
      current.reportDeltaCelsius !== original.reportDeltaCelsius ||
      current.reportAlways !== original.reportAlways ||
      current.enabled !== original.enabled
    )
  }

  export function encodeConfig(config: ConfigDraft): Record<string, unknown> {
    return {
      name: config.name,
      enabled: config.enabled,
      deps: config.deps,
      address: config.address.trim().toUpperCase(),
      resolution: config.resolution,
      unit: config.unit,
      pollMs: config.pollMs,
      reportDeltaCelsius: config.reportDeltaCelsius,
      reportAlways: config.reportAlways,
    }
  }

  export function addressValid(address: string): boolean {
    return /^[0-9A-Fa-f]{16}$/.test(address.trim())
  }

  export function isScanCandidate(candidate: OneWireScanDeviceSnapshot): boolean {
    return candidate.familyCode.toUpperCase() === '28' && addressValid(candidate.address)
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
    return `${output.value.toFixed(2)} ${output.unitSymbol}`
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
      return Ds18b20.normalizeConfig(value, deps)
    }

    normalizeOutput(record: DeviceRecord): Ds18b20TemperatureSensorOutputSnapshot {
      return Ds18b20.normalizeOutput(record.runtime)
    }

    override encodeConfig(config: ConfigDraft): Record<string, unknown> {
      return Ds18b20.encodeConfig(config)
    }

    buildEditCommands(current: DeviceRecord, draft: CreateDraft): DeviceCommandRequest[] {
      const commands: DeviceCommandRequest[] = []
      const currentConfig = this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined)
      if (draft.name.trim() !== currentConfig.name) {
        commands.push({ command: 'rename', name: draft.name.trim() })
      }
      if (draft.enabled !== currentConfig.enabled) {
        commands.push({ command: draft.enabled ? 'enable' : 'disable' })
      }
      const nextConfig = this.normalizeConfig(draft, [
        {
          role: 'onewire_bus',
          deviceId: draft.dependencyDeviceId,
        },
      ])
      if (Ds18b20.configChanged(nextConfig, currentConfig)) {
        commands.push({
          command: 'updateConfig',
          config: {
            ...this.encodeConfig(nextConfig),
            enabled: draft.enabled,
          },
          deps: [
            {
              role: 'onewire_bus',
              deviceId: nextConfig.dependencyDeviceId,
            },
          ],
        })
      }
      return commands
    }

    protected extractCreateConfig(draft: CreateDraft): ConfigDraft {
      const { typeId: _typeId, ...config } = draft
      return config
    }

    protected override createCreateDeps(config: ConfigDraft) {
      return [
        {
          role: 'onewire_bus',
          deviceId: config.dependencyDeviceId,
        },
      ]
    }
  }
}
