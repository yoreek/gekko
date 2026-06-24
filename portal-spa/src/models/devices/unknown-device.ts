import { BaseDevice } from '@/models/devices/base-device'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'

export interface UnknownDeviceCreateDraft extends DeviceCreateDraftBase {
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
      typeName: common.typeName ?? '',
      enabled: common.enabled ?? true,
      deps: common.deps ?? [],
    }
  }

  createEditDraft(current: DeviceRecord): UnknownDeviceCreateDraft {
    const config = current.config ?? {}
    return {
      name: config.name ?? 'New Device',
      typeName: current.record?.typeName ?? '',
      enabled: config.enabled ?? true,
      deps: Array.isArray(config.deps) ? config.deps : [],
    }
  }

  normalizeConfig(_value: unknown): Record<string, never> {
    return {}
  }

  normalizeOutput(_record: DeviceRecord): Record<string, unknown> {
    return {}
  }

  buildEditCommands(current: DeviceRecord, draft: UnknownDeviceCreateDraft): DeviceCommandRequest[] {
    const config = current.config ?? {}
    const commands: DeviceCommandRequest[] = []
    if (draft.name.trim() !== config.name) {
      commands.push({ command: 'rename', name: draft.name.trim() })
    }
    if (draft.enabled !== config.enabled) {
      commands.push({ command: draft.enabled ? 'enable' : 'disable' })
    }
    return commands
  }

  protected extractCreateConfig(_draft: UnknownDeviceCreateDraft): Record<string, never> {
    return {}
  }
}
