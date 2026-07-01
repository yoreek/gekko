import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice } from './base-device.ts'
import type { BaseDeviceConfig } from '@/api/contracts'

export type DummyConfigDraft = BaseDeviceConfig

export interface DummyCreateDraft extends DeviceCreateDraftBase {}

export class DummyDevice extends BaseDevice<DummyConfigDraft, DummyCreateDraft, DummyConfigDraft> {
  static readonly TYPE_ID = 1 as const
  static readonly TYPE_NAME = 'dummy' as const

  readonly typeName = DummyDevice.TYPE_NAME
  readonly typeId = DummyDevice.TYPE_ID

  static defaultConfig(): DummyConfigDraft {
    return {
      name: 'New Device',
      enabled: true,
      deps: [],
    }
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
    if (draft.name.trim() !== config?.name) {
      commands.push({ command: 'rename', name: draft.name.trim() })
    }
    if (draft.enabled !== config?.enabled) {
      commands.push({ command: draft.enabled ? 'enable' : 'disable' })
    }
    return commands
  }

  protected extractCreateConfig(_draft: DummyCreateDraft): DummyConfigDraft {
    return DummyDevice.defaultConfig()
  }
}
