import type { DeviceRecord } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceRole } from '@/models/device-type-ids'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'
import type { BaseDeviceConfig } from '@/api/contracts'
import { normalizePin } from './shared/pin.ts'

export interface OneWireBusConfigDraft extends BaseDeviceConfig {
  gpioPin: number
  internalPullup: boolean
}

export interface OneWireBusCreateDraft extends DeviceCreateDraftBase, OneWireBusConfigDraft {}

export class OneWireBusDevice extends BaseDevice<OneWireBusConfigDraft, OneWireBusCreateDraft, Record<string, never>> {
  static readonly TYPE_ID = 3 as const
  static readonly TYPE_NAME = 'onewire_bus' as const

  readonly typeName = OneWireBusDevice.TYPE_NAME
  readonly typeId = OneWireBusDevice.TYPE_ID
  readonly dependencyRoles: DeviceRole[] = ['onewire_bus']

  static defaultConfig(): OneWireBusConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      gpioPin: 255,
      internalPullup: false,
    }
  }

  static normalizeConfig(value: unknown): OneWireBusConfigDraft {
    const defaults = OneWireBusDevice.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return defaults
    }
    const raw = value as Record<string, unknown>
    return {
      ...normalizeBaseDeviceConfig(raw, defaults),
      gpioPin: normalizePin(raw.gpioPin, defaults.gpioPin, 'output'),
      internalPullup: typeof raw.internalPullup === 'boolean' ? raw.internalPullup : defaults.internalPullup,
    }
  }

  createDefaultConfig(): OneWireBusConfigDraft {
    return OneWireBusDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): OneWireBusCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): OneWireBusCreateDraft {
    return {
      ...this.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown): OneWireBusConfigDraft {
    return OneWireBusDevice.normalizeConfig(value)
  }

  normalizeOutput(_record: DeviceRecord): Record<string, never> {
    return {}
  }
}
