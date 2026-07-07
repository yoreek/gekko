import { DummyDevice } from './devices/dummy.ts'
import { GpioSwitchDevice } from './devices/gpio-switch.ts'
import { OneWireBusDevice } from './devices/onewire-bus.ts'
import { I2cBusDevice } from './devices/i2c-bus.ts'
import { Ds18b20Device } from './devices/ds18b20.ts'
import { NtcThermistorDevice } from './devices/ntc-thermistor.ts'
import { ThermostatDevice } from './devices/thermostat.ts'
import { SpiBusDevice } from './devices/spi-bus.ts'
import { Ssd1306Device } from './devices/ssd1306/device.ts'
import { St7735Device } from './devices/st7735/device.ts'

export const DUMMY_DEVICE_TYPE_ID = DummyDevice.TYPE_ID
export const GPIO_SWITCH_DEVICE_TYPE_ID = GpioSwitchDevice.TYPE_ID
export const ONEWIRE_BUS_DEVICE_TYPE_ID = OneWireBusDevice.TYPE_ID
export const I2C_BUS_DEVICE_TYPE_ID = I2cBusDevice.TYPE_ID
export const DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID = Ds18b20Device.TYPE_ID
export const NTC_THERMISTOR_TEMPERATURE_SENSOR_DEVICE_TYPE_ID = NtcThermistorDevice.TYPE_ID
export const THERMOSTAT_DEVICE_TYPE_ID = ThermostatDevice.TYPE_ID
export const SPI_BUS_DEVICE_TYPE_ID = SpiBusDevice.TYPE_ID
export const SSD1306_DEVICE_TYPE_ID = Ssd1306Device.TYPE_ID
export const ST7735_DEVICE_TYPE_ID = St7735Device.TYPE_ID

export type DeviceTypeName =
  | typeof DummyDevice.TYPE_NAME
  | typeof GpioSwitchDevice.TYPE_NAME
  | typeof OneWireBusDevice.TYPE_NAME
  | typeof I2cBusDevice.TYPE_NAME
  | typeof Ds18b20Device.TYPE_NAME
  | typeof NtcThermistorDevice.TYPE_NAME
  | typeof ThermostatDevice.TYPE_NAME
  | typeof SpiBusDevice.TYPE_NAME
  | typeof Ssd1306Device.TYPE_NAME
  | typeof St7735Device.TYPE_NAME
export type DeviceTypeId = number

const deviceTypeIds: Record<DeviceTypeName, DeviceTypeId> = {
  [DummyDevice.TYPE_NAME]: DummyDevice.TYPE_ID,
  [GpioSwitchDevice.TYPE_NAME]: GpioSwitchDevice.TYPE_ID,
  [OneWireBusDevice.TYPE_NAME]: OneWireBusDevice.TYPE_ID,
  [I2cBusDevice.TYPE_NAME]: I2cBusDevice.TYPE_ID,
  [Ds18b20Device.TYPE_NAME]: Ds18b20Device.TYPE_ID,
  [NtcThermistorDevice.TYPE_NAME]: NtcThermistorDevice.TYPE_ID,
  [ThermostatDevice.TYPE_NAME]: ThermostatDevice.TYPE_ID,
  [SpiBusDevice.TYPE_NAME]: SpiBusDevice.TYPE_ID,
  [Ssd1306Device.TYPE_NAME]: Ssd1306Device.TYPE_ID,
  [St7735Device.TYPE_NAME]: St7735Device.TYPE_ID,
}

const deviceTypeNames: Record<DeviceTypeId, DeviceTypeName> = {
  [DummyDevice.TYPE_ID]: DummyDevice.TYPE_NAME,
  [GpioSwitchDevice.TYPE_ID]: GpioSwitchDevice.TYPE_NAME,
  [OneWireBusDevice.TYPE_ID]: OneWireBusDevice.TYPE_NAME,
  [I2cBusDevice.TYPE_ID]: I2cBusDevice.TYPE_NAME,
  [Ds18b20Device.TYPE_ID]: Ds18b20Device.TYPE_NAME,
  [NtcThermistorDevice.TYPE_ID]: NtcThermistorDevice.TYPE_NAME,
  [ThermostatDevice.TYPE_ID]: ThermostatDevice.TYPE_NAME,
  [SpiBusDevice.TYPE_ID]: SpiBusDevice.TYPE_NAME,
  [Ssd1306Device.TYPE_ID]: Ssd1306Device.TYPE_NAME,
  [St7735Device.TYPE_ID]: St7735Device.TYPE_NAME,
}

export function deviceTypeIdFromName(typeName: string | undefined | null): DeviceTypeId {
  return typeName && typeName in deviceTypeIds ? deviceTypeIds[typeName as DeviceTypeName] : 0
}

export function deviceTypeName(typeId: number): DeviceTypeName {
  return deviceTypeNames[typeId] ?? DummyDevice.TYPE_NAME
}
