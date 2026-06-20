import type { Component } from 'vue'

import {
  DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
  DUMMY_DEVICE_TYPE_ID,
  GPIO_SWITCH_DEVICE_TYPE_ID,
  ONEWIRE_BUS_DEVICE_TYPE_ID,
  THERMOSTAT_DEVICE_TYPE_ID,
  resolveDeviceComponent,
  type DeviceTypeId,
} from '@/models/device-types'
import DummyDeviceDetail from '@/components/devices/dummy/DummyDeviceDetail.vue'
import Ds18b20TemperatureSensorDeviceDetail from '@/components/devices/ds18b20/Ds18b20TemperatureSensorDeviceDetail.vue'
import Ds18b20TemperatureSensorDeviceForm from '@/components/devices/ds18b20/Ds18b20TemperatureSensorDeviceForm.vue'
import GpioSwitchDeviceDetail from '@/components/devices/gpio-switch/GpioSwitchDeviceDetail.vue'
import GpioSwitchDeviceForm from '@/components/devices/gpio-switch/GpioSwitchDeviceForm.vue'
import OneWireBusDeviceDetail from '@/components/devices/onewire-bus/OneWireBusDeviceDetail.vue'
import OneWireBusDeviceForm from '@/components/devices/onewire-bus/OneWireBusDeviceForm.vue'
import ThermostatDeviceDetail from '@/components/devices/thermostat/ThermostatDeviceDetail.vue'
import ThermostatDeviceForm from '@/components/devices/thermostat/ThermostatDeviceForm.vue'

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
  {
    typeId: ONEWIRE_BUS_DEVICE_TYPE_ID,
    detail: OneWireBusDeviceDetail,
    form: OneWireBusDeviceForm,
  },
  {
    typeId: DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
    detail: Ds18b20TemperatureSensorDeviceDetail,
    form: Ds18b20TemperatureSensorDeviceForm,
  },
  {
    typeId: THERMOSTAT_DEVICE_TYPE_ID,
    detail: ThermostatDeviceDetail,
    form: ThermostatDeviceForm,
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
