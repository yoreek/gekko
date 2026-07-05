import type { DeviceCommandRequest, DeviceRecord, I2cBusRuntimeSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice } from './base-device.ts'
import type { BaseDeviceConfig } from '@/api/contracts'

export interface I2cBusConfigDraft extends BaseDeviceConfig {
  sdaPin: number
  sclPin: number
  internalPullup: boolean
  frequencyHz: number
}

export interface I2cBusCreateDraft extends DeviceCreateDraftBase, I2cBusConfigDraft {}

export class I2cBusDevice extends BaseDevice<I2cBusConfigDraft, I2cBusCreateDraft, I2cBusRuntimeSnapshot> {
  static readonly TYPE_ID = 6 as const
  static readonly TYPE_NAME = 'i2c_bus' as const

  readonly typeName = I2cBusDevice.TYPE_NAME
  readonly typeId = I2cBusDevice.TYPE_ID

  static defaultConfig(): I2cBusConfigDraft {
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

  static normalizeConfig(value: unknown): I2cBusConfigDraft {
    const defaults = I2cBusDevice.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return defaults
    }
    const raw = value as Record<string, unknown>
    return {
      name: typeof raw.name === 'string' ? raw.name : defaults.name,
      enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
      deps: Array.isArray(raw.deps) ? (raw.deps as I2cBusConfigDraft['deps']) : defaults.deps,
      sdaPin: typeof raw.sdaPin === 'number' && Number.isFinite(raw.sdaPin) ? raw.sdaPin : defaults.sdaPin,
      sclPin: typeof raw.sclPin === 'number' && Number.isFinite(raw.sclPin) ? raw.sclPin : defaults.sclPin,
      internalPullup: typeof raw.internalPullup === 'boolean' ? raw.internalPullup : defaults.internalPullup,
      frequencyHz:
        typeof raw.frequencyHz === 'number' && Number.isFinite(raw.frequencyHz)
          ? raw.frequencyHz
          : defaults.frequencyHz,
    }
  }

  static encodeConfig(config: I2cBusConfigDraft): Record<string, unknown> {
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

  createDefaultConfig(): I2cBusConfigDraft {
    return I2cBusDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): I2cBusCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): I2cBusCreateDraft {
    return {
      ...this.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown): I2cBusConfigDraft {
    return I2cBusDevice.normalizeConfig(value)
  }

  normalizeOutput(record: DeviceRecord): I2cBusRuntimeSnapshot {
    return record.runtime as I2cBusRuntimeSnapshot
  }

  override encodeConfig(config: I2cBusConfigDraft): Record<string, unknown> {
    return I2cBusDevice.encodeConfig(config)
  }

  buildEditCommands(current: DeviceRecord, draft: I2cBusCreateDraft): DeviceCommandRequest[] {
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

  protected extractCreateConfig(draft: I2cBusCreateDraft): I2cBusConfigDraft {
    return { ...draft }
  }
}
