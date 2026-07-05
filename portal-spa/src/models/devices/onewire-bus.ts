import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice } from './base-device.ts'
import type { BaseDeviceConfig } from '@/api/contracts'

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

  static defaultConfig(): OneWireBusConfigDraft {
    return {
      enabled: true,
      name: 'New Device',
      deps: [],
      gpioPin: 4,
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
      name: typeof raw.name === 'string' ? raw.name : defaults.name,
      enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
      deps: Array.isArray(raw.deps) ? (raw.deps as OneWireBusConfigDraft['deps']) : defaults.deps,
      gpioPin: typeof raw.gpioPin === 'number' && Number.isFinite(raw.gpioPin) ? raw.gpioPin : defaults.gpioPin,
      internalPullup: typeof raw.internalPullup === 'boolean' ? raw.internalPullup : defaults.internalPullup,
    }
  }

  static encodeConfig(config: OneWireBusConfigDraft): Record<string, unknown> {
    return {
      name: config.name,
      enabled: config.enabled,
      deps: config.deps,
      gpioPin: config.gpioPin,
      internalPullup: config.internalPullup,
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

  override encodeConfig(config: OneWireBusConfigDraft): Record<string, unknown> {
    return OneWireBusDevice.encodeConfig(config)
  }

  buildEditCommands(current: DeviceRecord, draft: OneWireBusCreateDraft): DeviceCommandRequest[] {
    const currentConfig = this.normalizeConfig(current.config)
    const commands: DeviceCommandRequest[] = []
    const nextConfig = this.normalizeConfig({ ...draft, name: draft.name.trim() })
    const configDiff = this.buildConfigDiff(this.encodeConfig(nextConfig), this.encodeConfig(currentConfig))
    if (Object.keys(configDiff).length > 0) {
      commands.push({
        command: 'updateConfig',
        config: configDiff,
      })
    }
    return commands
  }

  protected extractCreateConfig(draft: OneWireBusCreateDraft): OneWireBusConfigDraft {
    return { ...draft }
  }
}
