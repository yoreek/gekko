import type { DeviceRecord } from '@/api/contracts'
import { deviceTypeIdFromName } from '../device-types.ts'
import { BaseDevice } from './base-device.ts'
import { Ds18b20 } from './ds18b20.ts'
import { Dummy } from './dummy.ts'
import { GpioSwitch } from './gpio-switch.ts'
import { I2cBus } from './i2c-bus.ts'
import { Device as Ssd1306Device } from './ssd1306/device.ts'
import { Device as St7735Device } from './st7735/device.ts'
import { OneWireBus } from './onewire-bus.ts'
import { Thermostat } from './thermostat.ts'
import { UnknownDevice } from './unknown-device.ts'

const fallbackDevice = new UnknownDevice()

const deviceModelsByTypeId: Record<number, BaseDevice<any, any, any>> = {
  [deviceTypeIdFromName('dummy')]: new Dummy.Device(),
  [deviceTypeIdFromName('gpio_switch')]: new GpioSwitch.Device(),
  [deviceTypeIdFromName('onewire_bus')]: new OneWireBus.Device(),
  [deviceTypeIdFromName('i2c_bus')]: new I2cBus.Device(),
  [deviceTypeIdFromName('ssd1306')]: new Ssd1306Device(),
  [deviceTypeIdFromName('st7735')]: new St7735Device(),
  [deviceTypeIdFromName('ds18b20_temperature_sensor')]: new Ds18b20.Device(),
  [deviceTypeIdFromName('thermostat')]: new Thermostat.Device(),
}

export function resolveDeviceModelByTypeId(typeId: number): BaseDevice<any, any, any> {
  return deviceModelsByTypeId[typeId] ?? fallbackDevice
}

export function resolveDeviceModelByTypeName(typeName: string | undefined | null): BaseDevice<any, any, any> {
  return resolveDeviceModelByTypeId(deviceTypeIdFromName(typeName))
}

export function resolveDeviceModel(record: DeviceRecord | Record<string, unknown>): BaseDevice<any, any, any> {
  const source = record as Record<string, any>
  const typeName = typeof source.record?.typeName === 'string' && source.record.typeName.length > 0
    ? source.record.typeName
    : ''
  const typeId = deviceTypeIdFromName(typeName)
  return resolveDeviceModelByTypeId(typeId)
}
