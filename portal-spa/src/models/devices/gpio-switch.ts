import type { DeviceRecord, GpioSwitchOutputSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceRole } from '@/models/device-type-ids'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'
import type { SwitchConfigDraft } from '@/models/devices/switch-config'
import type { BaseDeviceConfig } from '@/api/contracts'

export interface GpioSwitchConfigDraft extends BaseDeviceConfig, SwitchConfigDraft {
  gpioPin: number
}

export interface GpioSwitchCreateDraft extends DeviceCreateDraftBase, GpioSwitchConfigDraft {}

function readSwitchState(value: unknown, fallback: boolean): boolean {
  return typeof value === 'boolean' ? value : fallback
}

export class GpioSwitchDevice extends BaseDevice<GpioSwitchConfigDraft, GpioSwitchCreateDraft, GpioSwitchOutputSnapshot> {
  static readonly TYPE_ID = 2 as const
  static readonly TYPE_NAME = 'gpio_switch' as const

  readonly typeName = GpioSwitchDevice.TYPE_NAME
  readonly typeId = GpioSwitchDevice.TYPE_ID
  readonly dependencyRoles: DeviceRole[] = ['switch', 'condition']

  static defaultConfig(): GpioSwitchConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      restorePreviousState: false,
      startupState: false,
      safeState: false,
      inverted: false,
      gpioPin: 4,
    }
  }

  static normalizeConfig(value: unknown): GpioSwitchConfigDraft {
    const defaults = GpioSwitchDevice.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return defaults
    }
    const raw = value as Record<string, unknown>
    const deps = Array.isArray(raw.deps) ? raw.deps.filter(dep => typeof dep === 'object' && dep !== null) as GpioSwitchConfigDraft['deps'] : defaults.deps
    return {
      ...normalizeBaseDeviceConfig(raw, defaults),
      deps,
      restorePreviousState:
        typeof raw.restorePreviousState === 'boolean' ? raw.restorePreviousState : defaults.restorePreviousState,
      startupState: readSwitchState(raw.startupState, defaults.startupState),
      safeState: readSwitchState(raw.safeState, defaults.safeState),
      inverted: typeof raw.inverted === 'boolean' ? raw.inverted : defaults.inverted,
      gpioPin: typeof raw.gpioPin === 'number' && Number.isFinite(raw.gpioPin) ? raw.gpioPin : defaults.gpioPin,
    }
  }

  createDefaultConfig(): GpioSwitchConfigDraft {
    return GpioSwitchDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): GpioSwitchCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): GpioSwitchCreateDraft {
    return {
      ...this.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown): GpioSwitchConfigDraft {
    return GpioSwitchDevice.normalizeConfig(value)
  }

  normalizeOutput(record: DeviceRecord): GpioSwitchOutputSnapshot {
    return record.runtime as GpioSwitchOutputSnapshot
  }
}
