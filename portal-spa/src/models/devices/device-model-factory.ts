import type { DeviceRecord } from '@/api/contracts'
import { deviceTypeIdFromName } from '../device-type-ids.ts'
import { BaseDevice } from './base-device.ts'
import { Ds18b20Device } from './ds18b20.ts'
import { DummyDevice } from './dummy.ts'
import { GpioSwitchDevice } from './gpio-switch.ts'
import { I2cBusDevice } from './i2c-bus.ts'
import { Ssd1306Device } from './ssd1306/device.ts'
import { St7735Device } from './st7735/device.ts'
import { SpiBusDevice } from './spi-bus.ts'
import { OneWireBusDevice } from './onewire-bus.ts'
import { ThermostatDevice } from './thermostat.ts'
import { UnknownDevice } from './unknown-device.ts'

const fallbackDevice = new UnknownDevice()

const allDeviceModels: BaseDevice<any, any, any>[] = [
  new DummyDevice(),
  new GpioSwitchDevice(),
  new OneWireBusDevice(),
  new I2cBusDevice(),
  new SpiBusDevice(),
  new Ssd1306Device(),
  new St7735Device(),
  new Ds18b20Device(),
  new ThermostatDevice(),
]

const deviceModelsByTypeId: Record<number, BaseDevice<any, any, any>> = Object.fromEntries(
  allDeviceModels.map(model => [model.typeId, model]),
)

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
