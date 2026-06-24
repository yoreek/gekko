import type { DeviceRecord } from '@/api/contracts'
import { deviceTypeIdFromName } from '@/models/device-types'
import { BaseDevice } from '@/models/devices/base-device'
import { Ds18b20 } from '@/models/devices/ds18b20'
import { Dummy } from '@/models/devices/dummy'
import { GpioSwitch } from '@/models/devices/gpio-switch'
import { OneWireBus } from '@/models/devices/onewire-bus'
import { Thermostat } from '@/models/devices/thermostat'
import { UnknownDevice } from '@/models/devices/unknown-device'

const fallbackDevice = new UnknownDevice()

const deviceModelsByTypeId: Record<number, BaseDevice<any, any, any>> = {
  [deviceTypeIdFromName('dummy')]: new Dummy.Device(),
  [deviceTypeIdFromName('gpio_switch')]: new GpioSwitch.Device(),
  [deviceTypeIdFromName('onewire_bus')]: new OneWireBus.Device(),
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
