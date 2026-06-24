import type { DeviceCommandRequest, DeviceRecord, OneWireScanSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { ONEWIRE_BUS_DEVICE_TYPE_ID } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'
import type { BaseDeviceConfig } from '@/api/contracts'

export namespace OneWireBus {
  export interface ConfigDraft extends BaseDeviceConfig {
    gpioPin: number
    internalPullup: boolean
  }

  export interface CreateDraft extends DeviceCreateDraftBase, ConfigDraft {}

  export function defaultConfig(): ConfigDraft {
    return {
      enabled: true,
      name: 'New Device',
      deps: [],
      gpioPin: 4,
      internalPullup: false,
    }
  }

  export function normalizeConfig(value: unknown): ConfigDraft {
    const defaults = defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return defaults
    }
    const raw = value as Record<string, unknown>
    return {
      name: typeof raw.name === 'string' ? raw.name : defaults.name,
      enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
      deps: Array.isArray(raw.deps) ? (raw.deps as ConfigDraft['deps']) : defaults.deps,
      gpioPin: typeof raw.gpioPin === 'number' && Number.isFinite(raw.gpioPin) ? raw.gpioPin : defaults.gpioPin,
      internalPullup: typeof raw.internalPullup === 'boolean' ? raw.internalPullup : defaults.internalPullup,
    }
  }

  export function encodeConfig(config: ConfigDraft): Record<string, unknown> {
    return {
      name: config.name,
      enabled: config.enabled,
      deps: config.deps,
      gpioPin: config.gpioPin,
      internalPullup: config.internalPullup,
    }
  }

  export class Device extends BaseDevice<ConfigDraft, CreateDraft, Record<string, never>> {
    readonly typeName = 'onewire_bus'
    readonly typeId = ONEWIRE_BUS_DEVICE_TYPE_ID

    createDefaultConfig(): ConfigDraft {
      return OneWireBus.defaultConfig()
    }

    createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): CreateDraft {
      return {
        ...this.createDefaultConfig(),
        ...common,
        typeName: common.typeName ?? this.typeName,
      }
    }

    createEditDraft(current: DeviceRecord): CreateDraft {
      return {
        ...this.normalizeConfig(current.config),
        typeName: this.typeName,
      }
    }

    normalizeConfig(value: unknown): ConfigDraft {
      return OneWireBus.normalizeConfig(value)
    }

    normalizeOutput(_record: DeviceRecord): Record<string, never> {
      return {}
    }

    override encodeConfig(config: ConfigDraft): Record<string, unknown> {
      return OneWireBus.encodeConfig(config)
    }

    buildEditCommands(current: DeviceRecord, draft: CreateDraft): DeviceCommandRequest[] {
      const currentConfig = this.normalizeConfig(current.config)
      const commands: DeviceCommandRequest[] = []
      if (draft.name.trim() !== currentConfig.name) {
        commands.push({ command: 'rename', name: draft.name.trim() })
      }
      if (draft.enabled !== currentConfig.enabled) {
        commands.push({ command: draft.enabled ? 'enable' : 'disable' })
      }
      const { enabled: _nextEnabled, ...nextConfig } = this.normalizeConfig(draft)
      if (
        nextConfig.gpioPin !== currentConfig.gpioPin ||
        nextConfig.internalPullup !== currentConfig.internalPullup
      ) {
        const encodedConfig = {
          ...nextConfig,
          enabled: draft.enabled,
        }
        commands.push({
          command: 'updateConfig',
          config: this.encodeConfig(encodedConfig),
        })
      }
      return commands
    }

    protected extractCreateConfig(draft: CreateDraft): ConfigDraft {
      return { ...draft }
    }
  }
}
