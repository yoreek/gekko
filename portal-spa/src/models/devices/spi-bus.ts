import type { BaseDeviceConfig, DeviceCommandRequest, DeviceRecord, SpiBusRuntimeSnapshot } from '../../api/contracts.ts'
import type { DeviceCreateDraftBase } from './base.ts'
import { BaseDevice } from './base-device.ts'

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
      enabled: true,
      name: 'New Device',
      deps: [],
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
      name: typeof raw.name === 'string' ? raw.name : defaults.name,
      enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
      deps: Array.isArray(raw.deps) ? (raw.deps as SpiBusConfigDraft['deps']) : defaults.deps,
      host: normalizeNumber(raw.host, defaults.host),
      sckPin: normalizeNumber(raw.sckPin, defaults.sckPin),
      mosiPin: normalizeNumber(raw.mosiPin, defaults.mosiPin),
      misoPin: normalizeNumber(raw.misoPin, defaults.misoPin),
    }
  }

  static encodeConfig(config: SpiBusConfigDraft): Record<string, unknown> {
    return {
      name: config.name,
      enabled: config.enabled,
      deps: config.deps,
      host: config.host,
      sckPin: config.sckPin,
      mosiPin: config.mosiPin,
      misoPin: config.misoPin,
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

  override encodeConfig(config: SpiBusConfigDraft): Record<string, unknown> {
    return SpiBusDevice.encodeConfig(config)
  }

  buildEditCommands(current: DeviceRecord, draft: SpiBusCreateDraft): DeviceCommandRequest[] {
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

  protected extractCreateConfig(draft: SpiBusCreateDraft): SpiBusConfigDraft {
    return { ...draft }
  }
}
