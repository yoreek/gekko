import type { DeviceRecord, GpioSwitchOutputSnapshot, DeviceDependencyLink, BaseDeviceConfig } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceRole } from '@/models/device-type-ids'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import type { SwitchConfigDraft } from '@/models/devices/switch-config'

export interface PortExpanderSwitchConfigDraft extends BaseDeviceConfig, SwitchConfigDraft {
  expanderDeviceId: number
  channel: number
}

export interface PortExpanderSwitchCreateDraft extends DeviceCreateDraftBase, PortExpanderSwitchConfigDraft {}

function readSwitchState(value: unknown, fallback: boolean): boolean {
  return typeof value === 'boolean' ? value : fallback
}

function deviceIdFromDeps(deps: DeviceDependencyLink[] | undefined, role: DeviceRole): number {
  return deps?.find(dep => dep.role === role)?.deviceId ?? 0
}

function normalizeDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

export class PortExpanderSwitchDevice extends BaseDevice<
  PortExpanderSwitchConfigDraft,
  PortExpanderSwitchCreateDraft,
  GpioSwitchOutputSnapshot
> {
  static readonly TYPE_ID = 14 as const
  static readonly TYPE_NAME = 'port_expander_switch' as const

  readonly typeName = PortExpanderSwitchDevice.TYPE_NAME
  readonly typeId = PortExpanderSwitchDevice.TYPE_ID
  readonly dependencyRoles: DeviceRole[] = ['switch', 'condition']

  static defaultConfig(): PortExpanderSwitchConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      restorePreviousState: false,
      startupState: false,
      safeState: false,
      inverted: false,
      expanderDeviceId: 0,
      channel: 0,
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): PortExpanderSwitchConfigDraft {
    const defaults = PortExpanderSwitchDevice.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return {
        ...defaults,
        deps: Array.isArray(deps) ? deps : defaults.deps,
        expanderDeviceId: deviceIdFromDeps(deps, 'port_expander'),
      }
    }
    const raw = value as Record<string, unknown>
    const rawDeps = Array.isArray(raw.deps)
      ? (raw.deps.filter(dep => typeof dep === 'object' && dep !== null) as DeviceDependencyLink[])
      : defaults.deps
    return {
      ...normalizeBaseDeviceConfig(raw, defaults),
      deps: rawDeps,
      restorePreviousState: typeof raw.restorePreviousState === 'boolean' ? raw.restorePreviousState : defaults.restorePreviousState,
      startupState: readSwitchState(raw.startupState, defaults.startupState),
      safeState: readSwitchState(raw.safeState, defaults.safeState),
      inverted: typeof raw.inverted === 'boolean' ? raw.inverted : defaults.inverted,
      expanderDeviceId: normalizeDeviceId(raw.expanderDeviceId ?? deviceIdFromDeps(deps ?? rawDeps, 'port_expander')),
      channel: typeof raw.channel === 'number' && Number.isFinite(raw.channel) ? raw.channel : defaults.channel,
    }
  }

  static encodeConfig(config: PortExpanderSwitchConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      restorePreviousState: config.restorePreviousState,
      startupState: config.startupState,
      safeState: config.safeState,
      inverted: config.inverted,
      channel: config.channel,
    }
  }

  createDefaultConfig(): PortExpanderSwitchConfigDraft {
    return PortExpanderSwitchDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): PortExpanderSwitchCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): PortExpanderSwitchCreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): PortExpanderSwitchConfigDraft {
    return PortExpanderSwitchDevice.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): GpioSwitchOutputSnapshot {
    return record.runtime as GpioSwitchOutputSnapshot
  }

  protected override encodeConfig(config: PortExpanderSwitchConfigDraft): Record<string, unknown> {
    return PortExpanderSwitchDevice.encodeConfig(config)
  }

  protected override createCreateDeps(config: PortExpanderSwitchConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'port_expander',
        deviceId: config.expanderDeviceId,
      },
    ]
  }
}
