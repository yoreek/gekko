import type { DeviceDependencyLink } from '@/api/contracts'
import type { DeviceTypeName } from '@/models/device-type-ids'
import { resolveDeviceModelByTypeName } from '@/models/devices/device-model-factory'

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
