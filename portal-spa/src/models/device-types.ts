import type { Component } from 'vue'

import type { DeviceOutputState } from '@/api'
import type { PortalIconName } from '@/icons'
import DummyDeviceWidget from '@/components/devices/dummy/DummyDeviceWidget.vue'
import GpioSwitchDeviceWidget from '@/components/devices/gpio-switch/GpioSwitchDeviceWidget.vue'
import I2cBusDeviceWidget from '@/components/devices/i2c-bus/I2cBusDeviceWidget.vue'
import OledDisplayDeviceWidget from '@/components/devices/oled-display/OledDisplayDeviceWidget.vue'
import OneWireBusDeviceWidget from '@/components/devices/onewire-bus/OneWireBusDeviceWidget.vue'
import Ds18b20TemperatureSensorDeviceWidget from '@/components/devices/ds18b20/Ds18b20TemperatureSensorDeviceWidget.vue'
import ThermostatDeviceWidget from '@/components/devices/thermostat/ThermostatDeviceWidget.vue'

export const DUMMY_DEVICE_TYPE_ID = 1 as const
export const GPIO_SWITCH_DEVICE_TYPE_ID = 2 as const
export const ONEWIRE_BUS_DEVICE_TYPE_ID = 3 as const
export const DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID = 4 as const
export const THERMOSTAT_DEVICE_TYPE_ID = 5 as const
export const I2C_BUS_DEVICE_TYPE_ID = 6 as const
export const OLED_DISPLAY_DEVICE_TYPE_ID = 7 as const

export type DeviceTypeId = number
export type DeviceTypeName =
  | 'dummy'
  | 'gpio_switch'
  | 'onewire_bus'
  | 'ds18b20_temperature_sensor'
  | 'thermostat'
  | 'i2c_bus'
  | 'oled_display'

export interface DeviceTypeOption {
  id: DeviceTypeId
  typeName: DeviceTypeName
  labelKey: string
  icon: PortalIconName
  componentKey: string
  supportedOutputStates?: DeviceOutputState[]
}

export interface DeviceComponentRegistryEntry {
  typeId: DeviceTypeId
  component: Component
}

export const deviceTypeOptions: DeviceTypeOption[] = [
  { id: DUMMY_DEVICE_TYPE_ID, typeName: 'dummy', labelKey: 'device.type.dummy', icon: 'device', componentKey: 'dummy' },
  {
    id: GPIO_SWITCH_DEVICE_TYPE_ID,
    typeName: 'gpio_switch',
    labelKey: 'device.type.gpioSwitch',
    icon: 'power',
    componentKey: 'gpio-switch',
    supportedOutputStates: ['off', 'on', 'disabled'],
  },
  {
    id: ONEWIRE_BUS_DEVICE_TYPE_ID,
    typeName: 'onewire_bus',
    labelKey: 'device.type.onewireBus',
    icon: 'bus',
    componentKey: 'onewire-bus',
  },
  {
    id: I2C_BUS_DEVICE_TYPE_ID,
    typeName: 'i2c_bus',
    labelKey: 'device.type.i2cBus',
    icon: 'bus',
    componentKey: 'i2c-bus',
  },
  {
    id: OLED_DISPLAY_DEVICE_TYPE_ID,
    typeName: 'oled_display',
    labelKey: 'device.type.oledDisplay',
    icon: 'device',
    componentKey: 'oled-display',
  },
  {
    id: DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
    typeName: 'ds18b20_temperature_sensor',
    labelKey: 'device.type.ds18b20TemperatureSensor',
    icon: 'temperature',
    componentKey: 'ds18b20-temperature-sensor',
  },
  {
    id: THERMOSTAT_DEVICE_TYPE_ID,
    typeName: 'thermostat',
    labelKey: 'device.type.thermostat',
    icon: 'temperature',
    componentKey: 'thermostat',
  },
]

const deviceTypeLabelKeys: Record<number, string> = {
  [DUMMY_DEVICE_TYPE_ID]: 'device.type.dummy',
  [GPIO_SWITCH_DEVICE_TYPE_ID]: 'device.type.gpioSwitch',
  [ONEWIRE_BUS_DEVICE_TYPE_ID]: 'device.type.onewireBus',
  [I2C_BUS_DEVICE_TYPE_ID]: 'device.type.i2cBus',
  [OLED_DISPLAY_DEVICE_TYPE_ID]: 'device.type.oledDisplay',
  [DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID]: 'device.type.ds18b20TemperatureSensor',
  [THERMOSTAT_DEVICE_TYPE_ID]: 'device.type.thermostat',
}

const deviceTypeNames: Record<number, DeviceTypeName> = {
  [DUMMY_DEVICE_TYPE_ID]: 'dummy',
  [GPIO_SWITCH_DEVICE_TYPE_ID]: 'gpio_switch',
  [ONEWIRE_BUS_DEVICE_TYPE_ID]: 'onewire_bus',
  [I2C_BUS_DEVICE_TYPE_ID]: 'i2c_bus',
  [OLED_DISPLAY_DEVICE_TYPE_ID]: 'oled_display',
  [DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID]: 'ds18b20_temperature_sensor',
  [THERMOSTAT_DEVICE_TYPE_ID]: 'thermostat',
}

const deviceTypeIds: Record<DeviceTypeName, DeviceTypeId> = {
  dummy: DUMMY_DEVICE_TYPE_ID,
  gpio_switch: GPIO_SWITCH_DEVICE_TYPE_ID,
  onewire_bus: ONEWIRE_BUS_DEVICE_TYPE_ID,
  i2c_bus: I2C_BUS_DEVICE_TYPE_ID,
  oled_display: OLED_DISPLAY_DEVICE_TYPE_ID,
  ds18b20_temperature_sensor: DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
  thermostat: THERMOSTAT_DEVICE_TYPE_ID,
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
    typeId: I2C_BUS_DEVICE_TYPE_ID,
    component: I2cBusDeviceWidget,
  },
  {
    typeId: OLED_DISPLAY_DEVICE_TYPE_ID,
    component: OledDisplayDeviceWidget,
  },
  {
    typeId: DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
    component: Ds18b20TemperatureSensorDeviceWidget,
  },
  {
    typeId: THERMOSTAT_DEVICE_TYPE_ID,
    component: ThermostatDeviceWidget,
  },
]

export function deviceTypeLabelKey(typeId: number): string {
  return deviceTypeLabelKeys[typeId] ?? 'device.type.unknown'
}

export function deviceTypeLabelKeyByName(typeName: string | undefined | null): string {
  return typeName && typeName in deviceTypeIds
    ? deviceTypeLabelKeys[deviceTypeIds[typeName as DeviceTypeName]] ?? 'device.type.unknown'
    : 'device.type.unknown'
}

export function deviceStatusLabelKey(status: string): string {
  switch (status.trim().toLowerCase()) {
    case 'ready':
      return 'device.status.ready'
    case 'disabled':
      return 'device.status.disabled'
    case 'dependency_blocked':
      return 'device.status.dependencyBlocked'
    case 'faulted':
      return 'device.status.faulted'
    case 'unknown':
      return 'device.status.unknown'
    default:
      return 'device.status.unknown'
  }
}

export function resolveDeviceTypeOption(typeId: number): DeviceTypeOption | undefined {
  return deviceTypeOptions.find(option => option.id === typeId)
}

export function resolveDeviceTypeOptionByName(typeName: string | undefined | null): DeviceTypeOption | undefined {
  return typeName && typeName in deviceTypeIds
    ? deviceTypeOptions.find(option => option.typeName === typeName)
    : undefined
}

export function deviceTypeName(typeId: number): DeviceTypeName {
  return deviceTypeNames[typeId] ?? 'dummy'
}

export function deviceTypeIdFromName(typeName: string | undefined | null): DeviceTypeId {
  return typeName && typeName in deviceTypeIds ? deviceTypeIds[typeName as DeviceTypeName] : 0
}

export function resolveDeviceComponent(typeId: number): Component {
  return deviceComponentRegistry.find(entry => entry.typeId === typeId)?.component
    ?? deviceComponentRegistry[0].component
}
