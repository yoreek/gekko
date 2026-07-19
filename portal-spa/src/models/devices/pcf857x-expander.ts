import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import type { BaseDeviceConfig, DeviceDependencyLink, DeviceRecord, PortExpanderOutputSnapshot } from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'

// PCF8574 and PCF8575 differ only in type id/name and channel count (8 vs 16); everything
// about their config shape and model behavior is shared here.
export interface Pcf857xExpanderConfigDraft extends BaseDeviceConfig {
  dependencyDeviceId: number
  i2cAddress: number
  inverted: boolean
}

export interface Pcf857xExpanderCreateDraft extends DeviceCreateDraftBase, Pcf857xExpanderConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeDependencyDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

export function defaultPcf857xExpanderConfig(): Pcf857xExpanderConfigDraft {
  return {
    ...defaultBaseDeviceConfig(),
    dependencyDeviceId: 0,
    i2cAddress: 0x20,
    inverted: false,
  }
}

export function normalizePcf857xExpanderConfig(
  value: unknown,
  dependencyDeviceOrDeps?: number | DeviceDependencyLink[],
): Pcf857xExpanderConfigDraft {
  const defaults = defaultPcf857xExpanderConfig()
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
  return {
    ...normalizeBaseDeviceConfig(value, defaults),
    dependencyDeviceId: normalizeDependencyDeviceId(dependencyDeviceId ?? value.dependencyDeviceId),
    i2cAddress: typeof value.i2cAddress === 'number' ? value.i2cAddress : defaults.i2cAddress,
    inverted: typeof value.inverted === 'boolean' ? value.inverted : defaults.inverted,
  }
}

export function encodePcf857xExpanderConfig(config: Pcf857xExpanderConfigDraft): Record<string, unknown> {
  return {
    ...encodeBaseDeviceConfig(config),
    i2cAddress: config.i2cAddress,
    inverted: config.inverted,
  }
}

export abstract class Pcf857xExpanderDeviceBase extends BaseDevice<
  Pcf857xExpanderConfigDraft,
  Pcf857xExpanderCreateDraft,
  PortExpanderOutputSnapshot
> {
  readonly dependencyRoles: DeviceRole[] = ['port_expander']

  createDefaultConfig(): Pcf857xExpanderConfigDraft {
    return defaultPcf857xExpanderConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Pcf857xExpanderCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): Pcf857xExpanderCreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Pcf857xExpanderConfigDraft {
    return normalizePcf857xExpanderConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): PortExpanderOutputSnapshot {
    return record.runtime as PortExpanderOutputSnapshot
  }

  protected override encodeConfig(config: Pcf857xExpanderConfigDraft): Record<string, unknown> {
    return encodePcf857xExpanderConfig(config)
  }

  protected override createCreateDeps(config: Pcf857xExpanderConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'i2c_bus',
        deviceId: config.dependencyDeviceId,
      },
    ]
  }
}
