import type { DeviceCommandRequest } from '@/api'
import type { DashboardDevice } from '@/models/device'
import { DUMMY_DEVICE_TYPE_ID } from '@/models/device-types'
import { resolveDeviceModel, resolveDeviceModelByTypeId } from '@/models/devices/device-model-factory'

export interface DeviceCommonDraft {
  name: string
  typeId: number
  enabled: boolean
}

export type DeviceCreateDraft = DeviceCommonDraft & Record<string, unknown>

export type DeviceEditDraft = DeviceCreateDraft

export function createDefaultDeviceDraft(typeId: number = DUMMY_DEVICE_TYPE_ID): DeviceCreateDraft {
  return resolveDeviceModelByTypeId(typeId).createDefaultCreateDraft({
    name: 'New Device',
    typeId,
    enabled: true,
  })
}

export function createDeviceEditDraft(device: DashboardDevice | null): DeviceEditDraft {
  if (device === null) {
    return createDefaultDeviceDraft()
  }
  return resolveDeviceModel(device.raw).createEditDraft(device)
}

export function buildDeviceEditCommands(device: DashboardDevice, payload: DeviceEditDraft): DeviceCommandRequest[] {
  return resolveDeviceModel(device.raw).buildEditCommands(device, payload)
}
