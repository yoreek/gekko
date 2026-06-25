import type { DeviceCommandRequest, DeviceRecord, I2cBusRuntimeSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { I2C_BUS_DEVICE_TYPE_ID } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'
import type { BaseDeviceConfig } from '@/api/contracts'

export namespace I2cBus {
  export interface ConfigDraft extends BaseDeviceConfig {
    sdaPin: number
    sclPin: number
    internalPullup: boolean
    frequencyHz: number
  }

  export interface CreateDraft extends DeviceCreateDraftBase, ConfigDraft {}

  export function defaultConfig(): ConfigDraft {
    return {
      enabled: true,
      name: 'New Device',
      deps: [],
      sdaPin: 21,
      sclPin: 22,
      internalPullup: true,
      frequencyHz: 100000,
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
      sdaPin: typeof raw.sdaPin === 'number' && Number.isFinite(raw.sdaPin) ? raw.sdaPin : defaults.sdaPin,
      sclPin: typeof raw.sclPin === 'number' && Number.isFinite(raw.sclPin) ? raw.sclPin : defaults.sclPin,
      internalPullup: typeof raw.internalPullup === 'boolean' ? raw.internalPullup : defaults.internalPullup,
      frequencyHz:
        typeof raw.frequencyHz === 'number' && Number.isFinite(raw.frequencyHz)
          ? raw.frequencyHz
          : defaults.frequencyHz,
    }
  }

  export function encodeConfig(config: ConfigDraft): Record<string, unknown> {
    return {
      name: config.name,
      enabled: config.enabled,
      deps: config.deps,
      sdaPin: config.sdaPin,
      sclPin: config.sclPin,
      internalPullup: config.internalPullup,
      frequencyHz: config.frequencyHz,
    }
  }

  export class Device extends BaseDevice<ConfigDraft, CreateDraft, I2cBusRuntimeSnapshot> {
    readonly typeName = 'i2c_bus'
    readonly typeId = I2C_BUS_DEVICE_TYPE_ID

    createDefaultConfig(): ConfigDraft {
      return I2cBus.defaultConfig()
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
      return I2cBus.normalizeConfig(value)
    }

    normalizeOutput(record: DeviceRecord): I2cBusRuntimeSnapshot {
      return record.runtime as I2cBusRuntimeSnapshot
    }

    override encodeConfig(config: ConfigDraft): Record<string, unknown> {
      return I2cBus.encodeConfig(config)
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
        nextConfig.sdaPin !== currentConfig.sdaPin ||
        nextConfig.sclPin !== currentConfig.sclPin ||
        nextConfig.internalPullup !== currentConfig.internalPullup ||
        nextConfig.frequencyHz !== currentConfig.frequencyHz
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
