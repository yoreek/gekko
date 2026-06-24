import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { DUMMY_DEVICE_TYPE_ID } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'
import type { BaseDeviceConfig } from '@/api/contracts'

export namespace Dummy {
  export type ConfigDraft = BaseDeviceConfig

  export interface CreateDraft extends DeviceCreateDraftBase {
    typeId: number
  }

  export function defaultConfig(): ConfigDraft {
    return {
      name: 'New Device',
      enabled: true,
      deps: [],
    }
  }

  export class Device extends BaseDevice<ConfigDraft, CreateDraft, ConfigDraft> {
    readonly typeName = 'dummy'
    readonly typeId = DUMMY_DEVICE_TYPE_ID

    createDefaultConfig(): ConfigDraft {
      return defaultConfig()
    }

    createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): CreateDraft {
      return {
        ...this.createDefaultConfig(),
        ...common,
        typeId: common.typeId ?? this.typeId,
      }
    }

    createEditDraft(current: DeviceRecord): CreateDraft {
      const config = current.config as Partial<ConfigDraft> | undefined
      return {
        ...this.createDefaultConfig(),
        name: config?.name ?? this.createDefaultConfig().name,
        enabled: config?.enabled ?? this.createDefaultConfig().enabled,
        deps: Array.isArray(config?.deps) ? config.deps : this.createDefaultConfig().deps,
        typeId: this.typeId,
      }
    }

    normalizeConfig(_value: unknown): ConfigDraft {
      return defaultConfig()
    }

    normalizeOutput(_record: DeviceRecord): ConfigDraft {
      return defaultConfig()
    }

    buildEditCommands(current: DeviceRecord, draft: CreateDraft): DeviceCommandRequest[] {
      const config = current.config as Partial<ConfigDraft> | undefined
      const commands: DeviceCommandRequest[] = []
      if (draft.name.trim() !== config?.name) {
        commands.push({ command: 'rename', name: draft.name.trim() })
      }
      if (draft.enabled !== config?.enabled) {
        commands.push({ command: draft.enabled ? 'enable' : 'disable' })
      }
      return commands
    }

    protected extractCreateConfig(_draft: CreateDraft): ConfigDraft {
      return defaultConfig()
    }
  }
}
