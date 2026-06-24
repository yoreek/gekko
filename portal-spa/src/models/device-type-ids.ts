export const DUMMY_DEVICE_TYPE_ID = 1 as const
export const GPIO_SWITCH_DEVICE_TYPE_ID = 2 as const
export const ONEWIRE_BUS_DEVICE_TYPE_ID = 3 as const
export const DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID = 4 as const
export const THERMOSTAT_DEVICE_TYPE_ID = 5 as const

export type DeviceTypeName = 'dummy' | 'gpio_switch' | 'onewire_bus' | 'ds18b20_temperature_sensor' | 'thermostat'
export type DeviceTypeId = number

const deviceTypeIds: Record<DeviceTypeName, DeviceTypeId> = {
  dummy: DUMMY_DEVICE_TYPE_ID,
  gpio_switch: GPIO_SWITCH_DEVICE_TYPE_ID,
  onewire_bus: ONEWIRE_BUS_DEVICE_TYPE_ID,
  ds18b20_temperature_sensor: DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
  thermostat: THERMOSTAT_DEVICE_TYPE_ID,
}

const deviceTypeNames: Record<DeviceTypeId, DeviceTypeName> = {
  [DUMMY_DEVICE_TYPE_ID]: 'dummy',
  [GPIO_SWITCH_DEVICE_TYPE_ID]: 'gpio_switch',
  [ONEWIRE_BUS_DEVICE_TYPE_ID]: 'onewire_bus',
  [DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID]: 'ds18b20_temperature_sensor',
  [THERMOSTAT_DEVICE_TYPE_ID]: 'thermostat',
}

export function deviceTypeIdFromName(typeName: string | undefined | null): DeviceTypeId {
  return typeName && typeName in deviceTypeIds ? deviceTypeIds[typeName as DeviceTypeName] : 0
}

export function deviceTypeName(typeId: number): DeviceTypeName {
  return deviceTypeNames[typeId] ?? 'dummy'
}
