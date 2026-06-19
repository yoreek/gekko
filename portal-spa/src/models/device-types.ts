import type { Component } from 'vue'

import type { DeviceOutputState } from '@/api'
import type { AppIconName } from '@/icons'
import DummyDeviceWidget from '@/components/devices/dummy/DummyDeviceWidget.vue'
import GpioSwitchDeviceWidget from '@/components/devices/gpio-switch/GpioSwitchDeviceWidget.vue'
import OneWireBusDeviceWidget from '@/components/devices/onewire-bus/OneWireBusDeviceWidget.vue'
import Ds18b20TemperatureSensorDeviceWidget from '@/components/devices/ds18b20/Ds18b20TemperatureSensorDeviceWidget.vue'

export const DUMMY_DEVICE_TYPE_ID = 1 as const
export const GPIO_SWITCH_DEVICE_TYPE_ID = 2 as const
export const ONEWIRE_BUS_DEVICE_TYPE_ID = 3 as const
export const DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID = 4 as const

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
  {
    id: ONEWIRE_BUS_DEVICE_TYPE_ID,
    labelKey: 'device.type.onewireBus',
    icon: 'bus',
    componentKey: 'onewire-bus',
  },
  {
    id: DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
    labelKey: 'device.type.ds18b20TemperatureSensor',
    icon: 'temperature',
    componentKey: 'ds18b20-temperature-sensor',
  },
]

const deviceTypeLabelKeys: Record<number, string> = {
  [DUMMY_DEVICE_TYPE_ID]: 'device.type.dummy',
  [GPIO_SWITCH_DEVICE_TYPE_ID]: 'device.type.gpioSwitch',
  [ONEWIRE_BUS_DEVICE_TYPE_ID]: 'device.type.onewireBus',
  [DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID]: 'device.type.ds18b20TemperatureSensor',
}

export const deviceComponentRegistry: DeviceComponentRegistryEntry[] = [
  {
    typeId: DUMMY_DEVICE_TYPE_ID,
    component: DummyDeviceWidget,
  },
  {
    typeId: GPIO_SWITCH_DEVICE_TYPE_ID,
    component: GpioSwitchDeviceWidget,
  },
  {
    typeId: ONEWIRE_BUS_DEVICE_TYPE_ID,
    component: OneWireBusDeviceWidget,
  },
  {
    typeId: DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
    component: Ds18b20TemperatureSensorDeviceWidget,
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
