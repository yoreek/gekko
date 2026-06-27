import type { BaseDeviceConfig, DeviceCommandRequest, DeviceRecord } from '../../api/contracts.ts'
import type { DeviceCreateDraftBase } from './base.ts'
import { BaseDevice } from './base-device.ts'
import { SPI_BUS_DEVICE_TYPE_ID } from '../device-type-ids.ts'

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

export function defaultSpiBusConfig(): SpiBusConfigDraft {
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

export function normalizeSpiBusConfig(value: unknown): SpiBusConfigDraft {
  const defaults = defaultSpiBusConfig()
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

export function encodeSpiBusConfig(config: SpiBusConfigDraft): Record<string, unknown> {
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

export class SpiBusDevice extends BaseDevice<SpiBusConfigDraft, SpiBusCreateDraft, Record<string, never>> {
  readonly typeName = 'spi_bus'
  readonly typeId = SPI_BUS_DEVICE_TYPE_ID

  createDefaultConfig(): SpiBusConfigDraft {
    return defaultSpiBusConfig()
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
    return normalizeSpiBusConfig(value)
  }

  normalizeOutput(): Record<string, never> {
    return {}
  }

  override encodeConfig(config: SpiBusConfigDraft): Record<string, unknown> {
    return encodeSpiBusConfig(config)
  }

  buildEditCommands(current: DeviceRecord, draft: SpiBusCreateDraft): DeviceCommandRequest[] {
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
      nextConfig.host !== currentConfig.host ||
      nextConfig.sckPin !== currentConfig.sckPin ||
      nextConfig.mosiPin !== currentConfig.mosiPin ||
      nextConfig.misoPin !== currentConfig.misoPin
    ) {
      commands.push({
        command: 'updateConfig',
        config: {
          ...this.encodeConfig(nextConfig),
          enabled: draft.enabled,
        },
      })
    }
    return commands
  }

  protected extractCreateConfig(draft: SpiBusCreateDraft): SpiBusConfigDraft {
    return { ...draft }
  }
}
