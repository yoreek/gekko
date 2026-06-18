import type { Component } from 'vue'

import {
  DUMMY_DEVICE_TYPE_ID,
  GPIO_SWITCH_DEVICE_TYPE_ID,
  resolveDeviceComponent,
  type DeviceTypeId,
} from '@/models/device-types'
import DummyDeviceDetail from '@/components/devices/dummy/DummyDeviceDetail.vue'
import GpioSwitchDeviceDetail from '@/components/devices/gpio-switch/GpioSwitchDeviceDetail.vue'
import GpioSwitchDeviceForm from '@/components/devices/gpio-switch/GpioSwitchDeviceForm.vue'

interface DeviceUiRegistryEntry {
  typeId: DeviceTypeId
  detail: Component
  form?: Component
}

const detailFallback = DummyDeviceDetail

const deviceUiRegistry: DeviceUiRegistryEntry[] = [
  {
    typeId: DUMMY_DEVICE_TYPE_ID,
    detail: DummyDeviceDetail,
  },
  {
    typeId: GPIO_SWITCH_DEVICE_TYPE_ID,
    detail: GpioSwitchDeviceDetail,
    form: GpioSwitchDeviceForm,
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
