import type { BaseDeviceConfig, DeviceRecord, SpiBusRuntimeSnapshot } from '../../api/contracts.ts'
import type { DeviceCreateDraftBase } from './base.ts'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'

export interface SpiBusConfigDraft extends BaseDeviceConfig {
  host: number
  sckPin: number
  mosiPin: number
  misoPin: number
}

export interface SpiBusCreateDraft extends DeviceCreateDraftBase, SpiBusConfigDraft {}

function normalizeNumber(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) ? numeric : fallback
}

export class SpiBusDevice extends BaseDevice<SpiBusConfigDraft, SpiBusCreateDraft, SpiBusRuntimeSnapshot> {
  static readonly TYPE_ID = 8 as const
  static readonly TYPE_NAME = 'spi_bus' as const

  readonly typeName = SpiBusDevice.TYPE_NAME
  readonly typeId = SpiBusDevice.TYPE_ID

  static defaultConfig(): SpiBusConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      host: 2,
      sckPin: 18,
      mosiPin: 23,
      misoPin: -1,
    }
  }

  static normalizeConfig(value: unknown): SpiBusConfigDraft {
    const defaults = SpiBusDevice.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return defaults
    }
    const raw = value as Record<string, unknown>
    return {
      ...normalizeBaseDeviceConfig(raw, defaults),
      host: normalizeNumber(raw.host, defaults.host),
      sckPin: normalizeNumber(raw.sckPin, defaults.sckPin),
      mosiPin: normalizeNumber(raw.mosiPin, defaults.mosiPin),
      misoPin: normalizeNumber(raw.misoPin, defaults.misoPin),
    }
  }

  createDefaultConfig(): SpiBusConfigDraft {
    return SpiBusDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): SpiBusCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): SpiBusCreateDraft {
    return {
      ...this.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown): SpiBusConfigDraft {
    return SpiBusDevice.normalizeConfig(value)
  }

  normalizeOutput(record: DeviceRecord): SpiBusRuntimeSnapshot {
    return record.runtime as SpiBusRuntimeSnapshot
  }
}
