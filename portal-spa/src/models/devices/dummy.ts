import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import type { DashboardDevice } from '@/models/device'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { DUMMY_DEVICE_TYPE_ID } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'

export namespace Dummy {
  export type ConfigDraft = Record<string, never>

  export interface CreateDraft extends DeviceCreateDraftBase {
    typeId: number
  }

  export function defaultConfig(): ConfigDraft {
    return {}
  }

  export class Device extends BaseDevice<ConfigDraft, CreateDraft, ConfigDraft> {
    readonly typeName = 'dummy'
    readonly typeId = DUMMY_DEVICE_TYPE_ID

    createDefaultConfig(): ConfigDraft {
      return defaultConfig()
    }

    createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): CreateDraft {
      return {
        name: common.name ?? 'New Device',
        typeId: common.typeId ?? this.typeId,
        enabled: common.enabled ?? true,
      }
    }

    createEditDraft(current: DashboardDevice): CreateDraft {
      return {
        name: current.name,
        typeId: current.typeId,
        enabled: current.enabled,
      }
    }

    normalizeConfig(_value: unknown): ConfigDraft {
      return {}
    }

    normalizeOutput(_record: DeviceRecord): ConfigDraft {
      return {}
    }

    buildEditCommands(current: DashboardDevice, draft: CreateDraft): DeviceCommandRequest[] {
      const commands: DeviceCommandRequest[] = []
      if (draft.name.trim() !== current.name) {
        commands.push({ command: 'rename', name: draft.name.trim() })
      }
      if (draft.enabled !== current.enabled) {
        commands.push({ command: draft.enabled ? 'enable' : 'disable' })
      }
      return commands
    }

    protected extractCreateConfig(_draft: CreateDraft): ConfigDraft {
      return {}
    }
  }
}
