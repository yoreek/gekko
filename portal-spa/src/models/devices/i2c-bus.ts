import type { DeviceRecord, I2cBusRuntimeSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceRole } from '@/models/device-type-ids'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'
import type { BaseDeviceConfig } from '@/api/contracts'
import { normalizePin } from './shared/pin.ts'

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
  readonly dependencyRoles: DeviceRole[] = ['i2c_bus']

  static defaultConfig(): I2cBusConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
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
      ...normalizeBaseDeviceConfig(raw, defaults),
      sdaPin: normalizePin(raw.sdaPin, defaults.sdaPin, 'output'),
      sclPin: normalizePin(raw.sclPin, defaults.sclPin, 'output'),
      internalPullup: typeof raw.internalPullup === 'boolean' ? raw.internalPullup : defaults.internalPullup,
      frequencyHz:
        typeof raw.frequencyHz === 'number' && Number.isFinite(raw.frequencyHz)
          ? raw.frequencyHz
          : defaults.frequencyHz,
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
}
