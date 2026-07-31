import type { DeviceRecord } from '@/api/contracts'
import { deviceTypeIdFromName, type DeviceRole } from '../device-type-ids.ts'
import { BaseDevice } from './base-device.ts'
import { Ds18b20Device } from './ds18b20.ts'
import { DummyDevice } from './dummy.ts'
import { GpioSwitchDevice } from './gpio-switch.ts'
import { I2cBusDevice } from './i2c-bus.ts'
import { Ssd1306Device } from './ssd1306/device.ts'
import { St7735Device } from './st7735/device.ts'
import { SpiBusDevice } from './spi-bus.ts'
import { NtcThermistorDevice } from './ntc-thermistor.ts'
import { Aht10Device } from './aht10.ts'
import { Dht11Device } from './dht11.ts'
import { Htu21Device } from './htu21.ts'
import { OneWireBusDevice } from './onewire-bus.ts'
import { ThermostatDevice } from './thermostat.ts'
import { RtcDs3231Device } from './rtc-ds3231.ts'
import { RtcDs1302Device } from './rtc-ds1302.ts'
import { Pcf8574ExpanderDevice } from './pcf8574-expander.ts'
import { Pcf8575ExpanderDevice } from './pcf8575-expander.ts'
import { PortExpanderSwitchDevice } from './port-expander-switch.ts'
import { ScheduleDevice } from './schedule.ts'
import { AutoSwitchDevice } from './auto-switch.ts'
import { BinarySensorDevice } from './binary-sensor.ts'
import { DosingPumpDevice } from './dosing-pump.ts'
import { AnalogOutputDevice } from './analog-output.ts'
import { AnalogOutputComposerDevice, FadeAnalogOutputDevice, ScheduledAnalogOutputDevice } from './composable-analog-output.ts'
import { AnalogPortInputDevice } from './analog-port-input.ts'
import { Ads1115HubDevice } from './ads1115-hub.ts'
import { Cd74hc4067HubDevice } from './cd74hc4067-hub.ts'
import { AnalogInputChannelDevice } from './analog-input-channel.ts'
import { Lcd1602Device } from './lcd1602.ts'
import { Lcd2004Device } from './lcd2004.ts'
import { Lcd1602PinDevice } from './lcd1602-pin.ts'
import { Lcd2004PinDevice } from './lcd2004-pin.ts'
import { Tm1637Device } from './tm1637.ts'
import { PixelStripDevice } from './pixel-strip.ts'
import { PixelEffectAlertDevice, PixelEffectSolidDevice } from './pixel-effects.ts'
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
  new NtcThermistorDevice(),
  new Aht10Device(),
  new Dht11Device(),
  new Htu21Device(),
  new ThermostatDevice(),
  new RtcDs3231Device(),
  new RtcDs1302Device(),
  new Pcf8574ExpanderDevice(),
  new Pcf8575ExpanderDevice(),
  new PortExpanderSwitchDevice(),
  new ScheduleDevice(),
  new AutoSwitchDevice(),
  new BinarySensorDevice(),
  new DosingPumpDevice(),
  new AnalogOutputDevice(),
  new FadeAnalogOutputDevice(),
  new ScheduledAnalogOutputDevice(),
  new AnalogOutputComposerDevice(),
  new AnalogPortInputDevice(),
  new Ads1115HubDevice(),
  new Cd74hc4067HubDevice(),
  new AnalogInputChannelDevice(),
  new Lcd1602Device(),
  new Lcd2004Device(),
  new Lcd1602PinDevice(),
  new Lcd2004PinDevice(),
  new Tm1637Device(),
  new PixelStripDevice(),
  new PixelEffectSolidDevice(),
  new PixelEffectAlertDevice(),
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

// Devices whose type can fill the given dependency role, so picker components list every
// compatible type instead of hardcoding one type id per dependency field.
export function devicesForDependencyRole(devices: DeviceRecord[], role: DeviceRole): DeviceRecord[] {
  return devices.filter(device => resolveDeviceModelByTypeName(device.record.typeName).dependencyRoles.includes(role))
}

export function dependencyOptionsForRole(devices: DeviceRecord[], role: DeviceRole): { title: string; value: number }[] {
  return devicesForDependencyRole(devices, role).map(device => ({
    title: `${device.config.name} #${device.record.id}`,
    value: device.record.id,
  }))
}

export function exclusiveAnalogOutputDependencyOptions(
  devices: DeviceRecord[],
  currentOwnerId = 0,
): { title: string; value: number }[] {
  const ownerTypes = new Set<string>([
    FadeAnalogOutputDevice.TYPE_NAME,
    ScheduledAnalogOutputDevice.TYPE_NAME,
    AnalogOutputComposerDevice.TYPE_NAME,
  ])
  const ownedTargets = new Map<number, number>()
  for (const device of devices) {
    if (!ownerTypes.has(device.record.typeName)) continue
    for (const dependency of device.config.deps ?? []) {
      if (dependency.role === 'analog_output') ownedTargets.set(dependency.deviceId, device.record.id)
    }
  }
  return dependencyOptionsForRole(devices, 'analog_output')
    .filter(option => option.value !== currentOwnerId)
    .filter(option => !ownedTargets.has(option.value) || ownedTargets.get(option.value) === currentOwnerId)
}

// Condition-role picker options (e.g. AutoSwitchDevice's AND-condition list), excluding devices
// already used elsewhere in the same config (typically the target switch/strip, to avoid a device
// conditioning on itself) and the device being edited itself -- some Condition-role providers
// (e.g. AutoSwitchDevice, via IStatusRuntime) can otherwise list their own id as a valid option,
// which the registry's self-dependency check would then reject at save time.
export function conditionDependencyOptions(devices: DeviceRecord[], ...excludeDeviceIds: (number | undefined)[]): { title: string; value: number }[] {
  const excluded = new Set(excludeDeviceIds.filter((id): id is number => typeof id === 'number' && id > 0))
  return dependencyOptionsForRole(devices, 'condition').filter(option => !excluded.has(option.value))
}

// pixel_strip's equivalent of exclusiveAnalogOutputDependencyOptions: a pixel_strip already
// claimed by one pixel_effect_* decorator (the registry's exclusive-controller rule, see
// docs/pixel-strip.md) is hidden from every other decorator's picker except its own current owner.
export function exclusivePixelStripDependencyOptions(
  devices: DeviceRecord[],
  currentOwnerId = 0,
): { title: string; value: number }[] {
  const ownerTypes = new Set<string>([PixelEffectSolidDevice.TYPE_NAME, PixelEffectAlertDevice.TYPE_NAME])
  const ownedTargets = new Map<number, number>()
  for (const device of devices) {
    if (!ownerTypes.has(device.record.typeName)) continue
    for (const dependency of device.config.deps ?? []) {
      if (dependency.role === 'pixel_strip') ownedTargets.set(dependency.deviceId, device.record.id)
    }
  }
  return dependencyOptionsForRole(devices, 'pixel_strip')
    .filter(option => option.value !== currentOwnerId)
    .filter(option => !ownedTargets.has(option.value) || ownedTargets.get(option.value) === currentOwnerId)
}
