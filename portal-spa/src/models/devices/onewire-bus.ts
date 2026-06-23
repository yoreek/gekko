import type { DeviceCommandRequest, DeviceRecord, OneWireScanSnapshot } from '@/api/contracts'
import type { DashboardDevice } from '@/models/device'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { ONEWIRE_BUS_DEVICE_TYPE_ID } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'

export namespace OneWireBus {
  export interface ConfigDraft {
    enabled: boolean
    gpio_pin: number
    internal_pullup: boolean
  }

  export interface CreateDraft extends DeviceCreateDraftBase, ConfigDraft {
    typeId: number
  }

  export function defaultConfig(): ConfigDraft {
    return {
      enabled: true,
      gpio_pin: 4,
      internal_pullup: false,
    }
  }

  export function normalizeConfig(value: unknown): ConfigDraft {
    const defaults = defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return defaults
    }
    const raw = value as Record<string, unknown>
    return {
      enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
      gpio_pin: typeof raw.gpio_pin === 'number' && Number.isFinite(raw.gpio_pin) ? raw.gpio_pin : defaults.gpio_pin,
      internal_pullup: typeof raw.internal_pullup === 'boolean' ? raw.internal_pullup : defaults.internal_pullup,
    }
  }

  export function encodeConfig(config: ConfigDraft): Record<string, unknown> {
    return {
      enabled: config.enabled,
      gpio_pin: config.gpio_pin,
      internal_pullup: config.internal_pullup,
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
        name: common.name ?? 'New Device',
        typeId: common.typeId ?? this.typeId,
        ...this.createDefaultConfig(),
        enabled: common.enabled ?? true,
      }
    }

    createEditDraft(current: DashboardDevice): CreateDraft {
      const { enabled: _enabled, ...config } = this.normalizeConfig(current.detail.config)
      return {
        name: current.name,
        typeId: current.typeId,
        enabled: current.enabled,
        ...config,
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

    buildEditCommands(current: DashboardDevice, draft: CreateDraft): DeviceCommandRequest[] {
      const commands: DeviceCommandRequest[] = []
      if (draft.name.trim() !== current.name) {
        commands.push({ command: 'rename', name: draft.name.trim() })
      }
      if (draft.enabled !== current.enabled) {
        commands.push({ command: draft.enabled ? 'enable' : 'disable' })
      }
      const { enabled: _currentEnabled, ...currentConfig } = this.normalizeConfig(current.detail.config)
      const { enabled: _nextEnabled, ...nextConfig } = this.normalizeConfig(draft)
      if (
        nextConfig.gpio_pin !== currentConfig.gpio_pin ||
        nextConfig.internal_pullup !== currentConfig.internal_pullup
      ) {
        const encodedConfig = {
          ...nextConfig,
          enabled: draft.enabled,
        }
        commands.push({
          command: 'update_config',
          config: this.encodeConfig(encodedConfig),
        })
      }
      return commands
    }

    protected extractCreateConfig(draft: CreateDraft): ConfigDraft {
      const { name: _name, typeId: _typeId, ...config } = draft
      return config
    }
  }
}
