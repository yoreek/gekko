import { BaseDevice } from '@/models/devices/base-device'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import type { DashboardDevice } from '@/models/device'

export interface UnknownDeviceCreateDraft extends DeviceCreateDraftBase {
  typeId: number
}

export class UnknownDevice extends BaseDevice<Record<string, unknown>, UnknownDeviceCreateDraft, Record<string, unknown>> {
  readonly typeName = ''
  readonly typeId = 0

  createDefaultConfig(): Record<string, never> {
    return {}
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): UnknownDeviceCreateDraft {
    return {
      name: common.name ?? 'New Device',
      typeId: common.typeId ?? 0,
      enabled: common.enabled ?? true,
    }
  }

  createEditDraft(current: DashboardDevice): UnknownDeviceCreateDraft {
    return {
      name: current.name,
      typeId: current.typeId,
      enabled: current.enabled,
    }
  }

  normalizeConfig(_value: unknown): Record<string, never> {
    return {}
  }

  normalizeOutput(_record: DeviceRecord): Record<string, unknown> {
    return {}
  }

  buildEditCommands(current: DashboardDevice, draft: UnknownDeviceCreateDraft): DeviceCommandRequest[] {
    const commands: DeviceCommandRequest[] = []
    if (draft.name.trim() !== current.name) {
      commands.push({ command: 'rename', name: draft.name.trim() })
    }
    if (draft.enabled !== current.enabled) {
      commands.push({ command: draft.enabled ? 'enable' : 'disable' })
    }
    return commands
  }

  protected extractCreateConfig(_draft: UnknownDeviceCreateDraft): Record<string, never> {
    return {}
  }
}
