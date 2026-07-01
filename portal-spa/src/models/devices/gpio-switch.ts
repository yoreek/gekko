import type { DeviceCommandRequest, DeviceRecord, GpioSwitchOutputSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice } from './base-device.ts'
import { outputStateOptions, type OutputState } from './switch.ts'
import type { SwitchConfigDraft } from '@/models/devices/switch-config'
import type { BaseDeviceConfig } from '@/api/contracts'

export interface GpioSwitchConfigDraft extends BaseDeviceConfig, SwitchConfigDraft {
  gpioPin: number
}

export interface GpioSwitchCreateDraft extends DeviceCreateDraftBase, GpioSwitchConfigDraft {}

function readOutputState(value: unknown, fallback: OutputState): OutputState {
  return value as OutputState ?? fallback
}

export class GpioSwitchDevice extends BaseDevice<GpioSwitchConfigDraft, GpioSwitchCreateDraft, GpioSwitchOutputSnapshot> {
  static readonly TYPE_ID = 2 as const
  static readonly TYPE_NAME = 'gpio_switch' as const

  readonly typeName = GpioSwitchDevice.TYPE_NAME
  readonly typeId = GpioSwitchDevice.TYPE_ID
  readonly supportedOutputStates = outputStateOptions

  static defaultConfig(): GpioSwitchConfigDraft {
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

  static normalizeConfig(value: unknown): GpioSwitchConfigDraft {
    const defaults = GpioSwitchDevice.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return defaults
    }
    const raw = value as Record<string, unknown>
    const deps = Array.isArray(raw.deps) ? raw.deps.filter(dep => typeof dep === 'object' && dep !== null) as GpioSwitchConfigDraft['deps'] : defaults.deps
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

  static encodeConfig(config: GpioSwitchConfigDraft): Record<string, unknown> {
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

  override encodeConfig(config: GpioSwitchConfigDraft): Record<string, unknown> {
    return GpioSwitchDevice.encodeConfig(config)
  }

  buildEditCommands(current: DeviceRecord, draft: GpioSwitchCreateDraft): DeviceCommandRequest[] {
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

  protected extractCreateConfig(draft: GpioSwitchCreateDraft): GpioSwitchConfigDraft {
    return { ...draft }
  }
}
