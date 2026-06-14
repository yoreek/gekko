import { defineAsyncComponent, type Component } from 'vue'

import {
  DUMMY_DEVICE_TYPE_ID,
  GPIO_SWITCH_DEVICE_TYPE_ID,
  resolveDeviceComponent,
  type DeviceTypeId,
} from '@/models/device-types'

interface DeviceUiRegistryEntry {
  typeId: DeviceTypeId
  detail: Component
  form?: Component
}

const detailFallback = defineAsyncComponent(() => import('@/components/devices/dummy/DummyDeviceDetail.vue'))

const deviceUiRegistry: DeviceUiRegistryEntry[] = [
  {
    typeId: DUMMY_DEVICE_TYPE_ID,
    detail: defineAsyncComponent(() => import('@/components/devices/dummy/DummyDeviceDetail.vue')),
  },
  {
    typeId: GPIO_SWITCH_DEVICE_TYPE_ID,
    detail: defineAsyncComponent(() => import('@/components/devices/gpio-switch/GpioSwitchDeviceDetail.vue')),
    form: defineAsyncComponent(() => import('@/components/devices/gpio-switch/GpioSwitchDeviceForm.vue')),
  },
]

export function resolveDashboardDeviceComponent(typeId: number): Component {
  return resolveDeviceComponent(typeId)
}

export function resolveDeviceDetailComponent(typeId: number): Component {
  return deviceUiRegistry.find(entry => entry.typeId === typeId)?.detail ?? detailFallback
}

export function resolveDeviceFormComponent(typeId: number): Component | undefined {
  return deviceUiRegistry.find(entry => entry.typeId === typeId)?.form
}

export function resolveDeviceCreateFormComponent(typeId: number): Component | undefined {
  return resolveDeviceFormComponent(typeId)
}
