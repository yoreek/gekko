import { defineAsyncComponent, type Component } from 'vue'

import type { DeviceOutputState } from '@/api'
import type { AppIconName } from '@/icons'

export const DUMMY_DEVICE_TYPE_ID = 1 as const
export const GPIO_SWITCH_DEVICE_TYPE_ID = 2 as const

export type DeviceTypeId = number

export interface DeviceTypeOption {
  id: DeviceTypeId
  labelKey: string
  icon: AppIconName
  componentKey: string
  supportedOutputStates?: DeviceOutputState[]
}

export interface DeviceComponentRegistryEntry {
  typeId: DeviceTypeId
  component: Component
}

export const deviceTypeOptions: DeviceTypeOption[] = [
  { id: DUMMY_DEVICE_TYPE_ID, labelKey: 'device.type.dummy', icon: 'device', componentKey: 'dummy' },
  {
    id: GPIO_SWITCH_DEVICE_TYPE_ID,
    labelKey: 'device.type.gpioSwitch',
    icon: 'power',
    componentKey: 'gpio-switch',
    supportedOutputStates: ['off', 'on', 'disabled'],
  },
]

const deviceTypeLabelKeys: Record<number, string> = {
  [DUMMY_DEVICE_TYPE_ID]: 'device.type.dummy',
  [GPIO_SWITCH_DEVICE_TYPE_ID]: 'device.type.gpioSwitch',
}

export const deviceComponentRegistry: DeviceComponentRegistryEntry[] = [
  {
    typeId: DUMMY_DEVICE_TYPE_ID,
    component: defineAsyncComponent(() => import('@/components/devices/dummy/DummyDeviceWidget.vue')),
  },
  {
    typeId: GPIO_SWITCH_DEVICE_TYPE_ID,
    component: defineAsyncComponent(() => import('@/components/devices/gpio-switch/GpioSwitchDeviceWidget.vue')),
  },
]

export function deviceTypeLabelKey(typeId: number): string {
  return deviceTypeLabelKeys[typeId] ?? 'device.type.unknown'
}

export function resolveDeviceTypeOption(typeId: number): DeviceTypeOption | undefined {
  return deviceTypeOptions.find(option => option.id === typeId)
}

export function resolveDeviceComponent(typeId: number): Component {
  return deviceComponentRegistry.find(entry => entry.typeId === typeId)?.component
    ?? deviceComponentRegistry[0].component
}
