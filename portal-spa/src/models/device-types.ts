import { defineAsyncComponent, type Component } from 'vue'

export const DUMMY_DEVICE_TYPE_ID = 1 as const

export type DeviceTypeId = number

export interface DeviceTypeOption {
  id: DeviceTypeId
  labelKey: string
}

export interface DeviceComponentRegistryEntry {
  typeId: DeviceTypeId
  component: Component
}

export const deviceTypeOptions: DeviceTypeOption[] = [
  { id: DUMMY_DEVICE_TYPE_ID, labelKey: 'device.type.dummy' },
]

const deviceTypeLabelKeys: Record<number, string> = {
  [DUMMY_DEVICE_TYPE_ID]: 'device.type.dummy',
}

export const deviceComponentRegistry: DeviceComponentRegistryEntry[] = [
  {
    typeId: DUMMY_DEVICE_TYPE_ID,
    component: defineAsyncComponent(() => import('@/components/devices/dummy/DummyDeviceWidget.vue')),
  },
]

export function deviceTypeLabelKey(typeId: number): string {
  return deviceTypeLabelKeys[typeId] ?? 'device.type.unknown'
}

export function resolveDeviceComponent(typeId: number): Component {
  return deviceComponentRegistry.find(entry => entry.typeId === typeId)?.component
    ?? deviceComponentRegistry[0].component
}
