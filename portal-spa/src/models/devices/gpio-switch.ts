import type { DeviceCommandRequest, DeviceRecord, GpioSwitchOutputSnapshot } from '@/api/contracts'
import type { DashboardDevice } from '@/models/device'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { GPIO_SWITCH_DEVICE_TYPE_ID } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'
import { isOutputState, type OutputState, type SwitchConfigDraft, normalizeSwitchOutput } from '@/models/devices/switch'

export namespace GpioSwitch {
  export interface ConfigDraft extends SwitchConfigDraft {
    gpio_pin: number
  }

  export interface CreateDraft extends DeviceCreateDraftBase, ConfigDraft {
    typeId: number
  }

  export function defaultConfig(): ConfigDraft {
    return {
      restore_previous_state: false,
      startup_state: 'off',
      safe_state: 'disabled',
      inverted: false,
      gpio_pin: 4,
    }
  }

  function readOutputState(value: unknown, fallback: OutputState): OutputState {
    return isOutputState(value) ? value : fallback
  }

  export function normalizeConfig(value: unknown): ConfigDraft {
    const defaults = defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return defaults
    }
    const raw = value as Record<string, unknown>
    return {
      restore_previous_state:
        typeof raw.restore_previous_state === 'boolean' ? raw.restore_previous_state : defaults.restore_previous_state,
      startup_state: readOutputState(raw.startup_state, defaults.startup_state),
      safe_state: readOutputState(raw.safe_state, defaults.safe_state),
      inverted: typeof raw.inverted === 'boolean' ? raw.inverted : defaults.inverted,
      gpio_pin: typeof raw.gpio_pin === 'number' && Number.isFinite(raw.gpio_pin) ? raw.gpio_pin : defaults.gpio_pin,
    }
  }

  export function encodeConfig(config: ConfigDraft): Record<string, unknown> {
    return {
      restore_previous_state: config.restore_previous_state,
      startup_state: config.startup_state,
      safe_state: config.safe_state,
      inverted: config.inverted,
      gpio_pin: config.gpio_pin,
    }
  }

  export class Device extends BaseDevice<ConfigDraft, CreateDraft, GpioSwitchOutputSnapshot> {
    readonly typeName = 'gpio_switch'
    readonly typeId = GPIO_SWITCH_DEVICE_TYPE_ID

    createDefaultConfig(): ConfigDraft {
      return GpioSwitch.defaultConfig()
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
      return {
        name: current.name,
        typeId: current.typeId,
        ...this.normalizeConfig(current.detail.config),
        enabled: current.enabled,
      }
    }

    normalizeConfig(value: unknown): ConfigDraft {
      return GpioSwitch.normalizeConfig(value)
    }

    normalizeOutput(record: DeviceRecord): GpioSwitchOutputSnapshot {
      return normalizeSwitchOutput(record.output)
    }

    override encodeConfig(config: ConfigDraft): Record<string, unknown> {
      return GpioSwitch.encodeConfig(config)
    }

    buildEditCommands(current: DashboardDevice, draft: CreateDraft): DeviceCommandRequest[] {
      const commands: DeviceCommandRequest[] = []
      if (draft.name.trim() !== current.name) {
        commands.push({ command: 'rename', name: draft.name.trim() })
      }
      if (draft.enabled !== current.enabled) {
        commands.push({ command: draft.enabled ? 'enable' : 'disable' })
      }
      const currentConfig = this.normalizeConfig(current.detail.config)
      const nextConfig = this.normalizeConfig(draft)
      if (
        nextConfig.restore_previous_state !== currentConfig.restore_previous_state ||
        nextConfig.startup_state !== currentConfig.startup_state ||
        nextConfig.safe_state !== currentConfig.safe_state ||
        nextConfig.inverted !== currentConfig.inverted ||
        nextConfig.gpio_pin !== currentConfig.gpio_pin
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
      const { name: _name, typeId: _typeId, enabled: _enabled, ...config } = draft
      return config
    }
  }
}
