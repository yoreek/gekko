import type { BaseDeviceConfig } from '@/api/contracts'

export interface DeviceCreateDraftBase extends BaseDeviceConfig {
  typeId: number
}
