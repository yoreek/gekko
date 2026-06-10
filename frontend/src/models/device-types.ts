export const DUMMY_DEVICE_TYPE_ID = 1 as const

export type DeviceTypeId = number

export interface DeviceTypeOption {
  id: DeviceTypeId
  labelKey: string
}

export const deviceTypeOptions: DeviceTypeOption[] = [
  { id: DUMMY_DEVICE_TYPE_ID, labelKey: 'device.type.dummy' },
]

const deviceTypeLabelKeys: Record<number, string> = {
  [DUMMY_DEVICE_TYPE_ID]: 'device.type.dummy',
}

export function deviceTypeLabelKey(typeId: number): string {
  return deviceTypeLabelKeys[typeId]
}
