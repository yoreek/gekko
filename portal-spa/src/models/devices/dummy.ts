import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig } from './base-device.ts'
import type { BaseDeviceConfig } from '@/api/contracts'

export type DummyConfigDraft = BaseDeviceConfig

export interface DummyCreateDraft extends DeviceCreateDraftBase {}

export class DummyDevice extends BaseDevice<DummyConfigDraft, DummyCreateDraft, DummyConfigDraft> {
  static readonly TYPE_ID = 1 as const
  static readonly TYPE_NAME = 'dummy' as const

  readonly typeName = DummyDevice.TYPE_NAME
  readonly typeId = DummyDevice.TYPE_ID

  static defaultConfig(): DummyConfigDraft {
    return defaultBaseDeviceConfig()
  }

  createDefaultConfig(): DummyConfigDraft {
    return DummyDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): DummyCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): DummyCreateDraft {
    const config = current.config as Partial<DummyConfigDraft> | undefined
    return {
      ...this.createDefaultConfig(),
      name: config?.name ?? this.createDefaultConfig().name,
      enabled: config?.enabled ?? this.createDefaultConfig().enabled,
      deps: Array.isArray(config?.deps) ? config.deps : this.createDefaultConfig().deps,
      typeName: this.typeName,
    }
  }

  normalizeConfig(_value: unknown): DummyConfigDraft {
    return DummyDevice.defaultConfig()
  }

  normalizeOutput(_record: DeviceRecord): DummyConfigDraft {
    return DummyDevice.defaultConfig()
  }

  buildEditCommands(current: DeviceRecord, draft: DummyCreateDraft): DeviceCommandRequest[] {
    const config = current.config as Partial<DummyConfigDraft> | undefined
    const commands: DeviceCommandRequest[] = []
    const configDiff: Record<string, unknown> = {}
    const trimmedName = draft.name.trim()
    if (trimmedName !== config?.name) {
      configDiff.name = trimmedName
    }
    if (draft.enabled !== config?.enabled) {
      configDiff.enabled = draft.enabled
    }
    if (Object.keys(configDiff).length > 0) {
      commands.push({ command: 'updateConfig', config: configDiff })
    }
    return commands
  }

  protected extractCreateConfig(_draft: DummyCreateDraft): DummyConfigDraft {
    return DummyDevice.defaultConfig()
  }
}
