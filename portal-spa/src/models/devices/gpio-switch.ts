import type { DeviceCommandRequest, DeviceRecord, GpioSwitchOutputSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { GPIO_SWITCH_DEVICE_TYPE_ID } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'
import { isOutputState, type OutputState, type SwitchConfigDraft, normalizeSwitchOutput } from '@/models/devices/switch'
import type { BaseDeviceConfig } from '@/api/contracts'

export namespace GpioSwitch {
  export interface ConfigDraft extends BaseDeviceConfig, SwitchConfigDraft {
    gpioPin: number
  }

  export interface CreateDraft extends DeviceCreateDraftBase, ConfigDraft {
    typeId: number
  }

  export function defaultConfig(): ConfigDraft {
    return {
      name: 'New Device',
      enabled: true,
      deps: [],
      restorePreviousState: false,
      startupState: 'off',
      safeState: 'disabled',
      inverted: false,
      gpioPin: 4,
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
    const deps = Array.isArray(raw.deps) ? raw.deps.filter(dep => typeof dep === 'object' && dep !== null) as ConfigDraft['deps'] : defaults.deps
    return {
      name: typeof raw.name === 'string' ? raw.name : defaults.name,
      enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
      deps,
      restorePreviousState:
        typeof raw.restorePreviousState === 'boolean' ? raw.restorePreviousState : defaults.restorePreviousState,
      startupState: readOutputState(raw.startupState, defaults.startupState),
      safeState: readOutputState(raw.safeState, defaults.safeState),
      inverted: typeof raw.inverted === 'boolean' ? raw.inverted : defaults.inverted,
      gpioPin: typeof raw.gpioPin === 'number' && Number.isFinite(raw.gpioPin) ? raw.gpioPin : defaults.gpioPin,
    }
  }

  export function encodeConfig(config: ConfigDraft): Record<string, unknown> {
    return {
      name: config.name,
      enabled: config.enabled,
      deps: config.deps,
      restorePreviousState: config.restorePreviousState,
      startupState: config.startupState,
      safeState: config.safeState,
      inverted: config.inverted,
      gpioPin: config.gpioPin,
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
        ...this.createDefaultConfig(),
        ...common,
        typeId: common.typeId ?? this.typeId,
      }
    }

    createEditDraft(current: DeviceRecord): CreateDraft {
      return {
        ...this.normalizeConfig(current.config),
        typeId: this.typeId,
      }
    }

    normalizeConfig(value: unknown): ConfigDraft {
      return GpioSwitch.normalizeConfig(value)
    }

    normalizeOutput(record: DeviceRecord): GpioSwitchOutputSnapshot {
      return normalizeSwitchOutput(record.runtime)
    }

    override encodeConfig(config: ConfigDraft): Record<string, unknown> {
      return GpioSwitch.encodeConfig(config)
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
      const nextConfig = this.normalizeConfig(draft)
      if (
        nextConfig.restorePreviousState !== currentConfig.restorePreviousState ||
        nextConfig.startupState !== currentConfig.startupState ||
        nextConfig.safeState !== currentConfig.safeState ||
        nextConfig.inverted !== currentConfig.inverted ||
        nextConfig.gpioPin !== currentConfig.gpioPin
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
      const { typeId: _typeId, ...config } = draft
      return config
    }
  }
}
