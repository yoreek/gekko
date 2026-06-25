import type { Component } from 'vue'

import {
  I2C_BUS_DEVICE_TYPE_ID,
  DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
  DUMMY_DEVICE_TYPE_ID,
  GPIO_SWITCH_DEVICE_TYPE_ID,
  ONEWIRE_BUS_DEVICE_TYPE_ID,
  THERMOSTAT_DEVICE_TYPE_ID,
  deviceTypeIdFromName,
  deviceTypeName,
  resolveDeviceComponent,
  type DeviceTypeId,
  type DeviceTypeName,
} from '@/models/device-types'
import DummyDeviceDetail from '@/components/devices/dummy/DummyDeviceDetail.vue'
import Ds18b20TemperatureSensorDeviceDetail from '@/components/devices/ds18b20/Ds18b20TemperatureSensorDeviceDetail.vue'
import Ds18b20TemperatureSensorDeviceForm from '@/components/devices/ds18b20/Ds18b20TemperatureSensorDeviceForm.vue'
import GpioSwitchDeviceDetail from '@/components/devices/gpio-switch/GpioSwitchDeviceDetail.vue'
import GpioSwitchDeviceForm from '@/components/devices/gpio-switch/GpioSwitchDeviceForm.vue'
import I2cBusDeviceDetail from '@/components/devices/i2c-bus/I2cBusDeviceDetail.vue'
import I2cBusDeviceForm from '@/components/devices/i2c-bus/I2cBusDeviceForm.vue'
import OneWireBusDeviceDetail from '@/components/devices/onewire-bus/OneWireBusDeviceDetail.vue'
import OneWireBusDeviceForm from '@/components/devices/onewire-bus/OneWireBusDeviceForm.vue'
import ThermostatDeviceDetail from '@/components/devices/thermostat/ThermostatDeviceDetail.vue'
import ThermostatDeviceForm from '@/components/devices/thermostat/ThermostatDeviceForm.vue'

interface DeviceUiRegistryEntry {
  typeId: DeviceTypeId
  typeName: DeviceTypeName
  detail: Component
  form?: Component
}

const detailFallback = DummyDeviceDetail

const deviceUiRegistry: DeviceUiRegistryEntry[] = [
  {
    typeId: DUMMY_DEVICE_TYPE_ID,
    typeName: deviceTypeName(DUMMY_DEVICE_TYPE_ID),
    detail: DummyDeviceDetail,
  },
  {
    typeId: GPIO_SWITCH_DEVICE_TYPE_ID,
    typeName: deviceTypeName(GPIO_SWITCH_DEVICE_TYPE_ID),
    detail: GpioSwitchDeviceDetail,
    form: GpioSwitchDeviceForm,
  },
  {
    typeId: ONEWIRE_BUS_DEVICE_TYPE_ID,
    typeName: deviceTypeName(ONEWIRE_BUS_DEVICE_TYPE_ID),
    detail: OneWireBusDeviceDetail,
    form: OneWireBusDeviceForm,
  },
  {
    typeId: I2C_BUS_DEVICE_TYPE_ID,
    typeName: deviceTypeName(I2C_BUS_DEVICE_TYPE_ID),
    detail: I2cBusDeviceDetail,
    form: I2cBusDeviceForm,
  },
  {
    typeId: DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
    typeName: deviceTypeName(DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID),
    detail: Ds18b20TemperatureSensorDeviceDetail,
    form: Ds18b20TemperatureSensorDeviceForm,
  },
  {
    typeId: THERMOSTAT_DEVICE_TYPE_ID,
    typeName: deviceTypeName(THERMOSTAT_DEVICE_TYPE_ID),
    detail: ThermostatDeviceDetail,
    form: ThermostatDeviceForm,
  },
]

function resolveEntry(typeIdOrName: number | string): DeviceUiRegistryEntry | undefined {
  return typeof typeIdOrName === 'number'
    ? deviceUiRegistry.find(entry => entry.typeId === typeIdOrName)
    : deviceUiRegistry.find(entry => entry.typeName === typeIdOrName)
}

export function resolveDeviceWidgetComponent(typeIdOrName: number | string): Component {
  return typeof typeIdOrName === 'number'
    ? resolveDeviceComponent(typeIdOrName)
    : resolveDeviceComponent(deviceTypeIdFromName(typeIdOrName))
}

export function resolveDeviceDetailComponent(typeIdOrName: number | string): Component {
  return resolveEntry(typeIdOrName)?.detail ?? detailFallback
}

export function resolveDeviceFormComponent(typeIdOrName: number | string): Component | undefined {
  return resolveEntry(typeIdOrName)?.form
}
