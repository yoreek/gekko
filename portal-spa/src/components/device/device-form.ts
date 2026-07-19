import type { DeviceDependencyLink } from '@/api/contracts'
import type { DeviceCommandRequest } from '@/api'
import type { DeviceRecord } from '@/api/contracts'
import type { DeviceTypeName } from '@/models/device-type-ids'
import { resolveDeviceModel, resolveDeviceModelByTypeName } from '@/models/devices/device-model-factory'

export interface DeviceCommonDraft {
  name: string
  typeName: DeviceTypeName
  enabled: boolean
  deps?: DeviceDependencyLink[]
}

export type DeviceCreateDraft = DeviceCommonDraft & Record<string, unknown>

export type DeviceEditDraft = DeviceCreateDraft

export function createDefaultDeviceDraft(typeName: DeviceTypeName = 'dummy'): DeviceCreateDraft {
  return resolveDeviceModelByTypeName(typeName).createDefaultCreateDraft({
    name: 'New Device',
    typeName,
    enabled: true,
  })
}

export function createDeviceEditDraft(device: DeviceRecord | null): DeviceEditDraft {
  if (device === null) {
    return createDefaultDeviceDraft()
  }
  return resolveDeviceModel(device).createEditDraft(device)
}

export function buildDeviceEditCommands(device: DeviceRecord, payload: DeviceEditDraft): DeviceCommandRequest[] {
  const commands = resolveDeviceModel(device).buildEditCommands(device, payload)
  const pendingCommands = Array.isArray(payload.pendingCommands)
    ? payload.pendingCommands as DeviceCommandRequest[]
    : []
  return [...commands, ...pendingCommands]
}
