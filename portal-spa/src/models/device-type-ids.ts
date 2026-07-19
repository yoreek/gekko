import { DummyDevice } from './devices/dummy.ts'
import { GpioSwitchDevice } from './devices/gpio-switch.ts'
import { OneWireBusDevice } from './devices/onewire-bus.ts'
import { I2cBusDevice } from './devices/i2c-bus.ts'
import { Ds18b20Device } from './devices/ds18b20.ts'
import { NtcThermistorDevice } from './devices/ntc-thermistor.ts'
import { Htu21Device } from './devices/htu21.ts'
import { ThermostatDevice } from './devices/thermostat.ts'
import { SpiBusDevice } from './devices/spi-bus.ts'
import { Ssd1306Device } from './devices/ssd1306/device.ts'
import { St7735Device } from './devices/st7735/device.ts'
import { RtcDs3231Device } from './devices/rtc-ds3231.ts'
import { Pcf8574ExpanderDevice } from './devices/pcf8574-expander.ts'
import { Pcf8575ExpanderDevice } from './devices/pcf8575-expander.ts'
import { PortExpanderSwitchDevice } from './devices/port-expander-switch.ts'
import { ScheduleDevice } from './devices/schedule.ts'
import { AutoSwitchDevice } from './devices/auto-switch.ts'
import { BinarySensorDevice } from './devices/binary-sensor.ts'
import { DosingPumpDevice } from './devices/dosing-pump.ts'
import { AnalogOutputDevice } from './devices/analog-output.ts'
import { AnalogOutputComposerDevice, FadeAnalogOutputDevice, ScheduledAnalogOutputDevice } from './devices/composable-analog-output.ts'

export const DUMMY_DEVICE_TYPE_ID = DummyDevice.TYPE_ID
export const GPIO_SWITCH_DEVICE_TYPE_ID = GpioSwitchDevice.TYPE_ID
export const ONEWIRE_BUS_DEVICE_TYPE_ID = OneWireBusDevice.TYPE_ID
export const I2C_BUS_DEVICE_TYPE_ID = I2cBusDevice.TYPE_ID
export const DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID = Ds18b20Device.TYPE_ID
export const NTC_THERMISTOR_TEMPERATURE_SENSOR_DEVICE_TYPE_ID = NtcThermistorDevice.TYPE_ID
export const HTU21_DEVICE_TYPE_ID = Htu21Device.TYPE_ID
export const THERMOSTAT_DEVICE_TYPE_ID = ThermostatDevice.TYPE_ID
export const SPI_BUS_DEVICE_TYPE_ID = SpiBusDevice.TYPE_ID
export const SSD1306_DEVICE_TYPE_ID = Ssd1306Device.TYPE_ID
export const ST7735_DEVICE_TYPE_ID = St7735Device.TYPE_ID
export const RTC_DS3231_DEVICE_TYPE_ID = RtcDs3231Device.TYPE_ID
export const PCF8574_EXPANDER_DEVICE_TYPE_ID = Pcf8574ExpanderDevice.TYPE_ID
export const PCF8575_EXPANDER_DEVICE_TYPE_ID = Pcf8575ExpanderDevice.TYPE_ID
export const PORT_EXPANDER_SWITCH_DEVICE_TYPE_ID = PortExpanderSwitchDevice.TYPE_ID
export const SCHEDULE_DEVICE_TYPE_ID = ScheduleDevice.TYPE_ID
export const AUTO_SWITCH_DEVICE_TYPE_ID = AutoSwitchDevice.TYPE_ID
export const BINARY_SENSOR_DEVICE_TYPE_ID = BinarySensorDevice.TYPE_ID
export const DOSING_PUMP_DEVICE_TYPE_ID = DosingPumpDevice.TYPE_ID
export const ANALOG_OUTPUT_DEVICE_TYPE_ID = AnalogOutputDevice.TYPE_ID
export const FADE_ANALOG_OUTPUT_DEVICE_TYPE_ID = FadeAnalogOutputDevice.TYPE_ID
export const SCHEDULED_ANALOG_OUTPUT_DEVICE_TYPE_ID = ScheduledAnalogOutputDevice.TYPE_ID
export const ANALOG_OUTPUT_COMPOSER_DEVICE_TYPE_ID = AnalogOutputComposerDevice.TYPE_ID

export type DeviceTypeName =
  | typeof DummyDevice.TYPE_NAME
  | typeof GpioSwitchDevice.TYPE_NAME
  | typeof OneWireBusDevice.TYPE_NAME
  | typeof I2cBusDevice.TYPE_NAME
  | typeof Ds18b20Device.TYPE_NAME
  | typeof NtcThermistorDevice.TYPE_NAME
  | typeof Htu21Device.TYPE_NAME
  | typeof ThermostatDevice.TYPE_NAME
  | typeof SpiBusDevice.TYPE_NAME
  | typeof Ssd1306Device.TYPE_NAME
  | typeof St7735Device.TYPE_NAME
  | typeof RtcDs3231Device.TYPE_NAME
  | typeof Pcf8574ExpanderDevice.TYPE_NAME
  | typeof Pcf8575ExpanderDevice.TYPE_NAME
  | typeof PortExpanderSwitchDevice.TYPE_NAME
  | typeof ScheduleDevice.TYPE_NAME
  | typeof AutoSwitchDevice.TYPE_NAME
  | typeof BinarySensorDevice.TYPE_NAME
  | typeof DosingPumpDevice.TYPE_NAME
  | typeof AnalogOutputDevice.TYPE_NAME
  | typeof FadeAnalogOutputDevice.TYPE_NAME
  | typeof ScheduledAnalogOutputDevice.TYPE_NAME
  | typeof AnalogOutputComposerDevice.TYPE_NAME
export type DeviceTypeId = number

const deviceTypeIds: Record<DeviceTypeName, DeviceTypeId> = {
  [DummyDevice.TYPE_NAME]: DummyDevice.TYPE_ID,
  [GpioSwitchDevice.TYPE_NAME]: GpioSwitchDevice.TYPE_ID,
  [OneWireBusDevice.TYPE_NAME]: OneWireBusDevice.TYPE_ID,
  [I2cBusDevice.TYPE_NAME]: I2cBusDevice.TYPE_ID,
  [Ds18b20Device.TYPE_NAME]: Ds18b20Device.TYPE_ID,
  [NtcThermistorDevice.TYPE_NAME]: NtcThermistorDevice.TYPE_ID,
  [Htu21Device.TYPE_NAME]: Htu21Device.TYPE_ID,
  [ThermostatDevice.TYPE_NAME]: ThermostatDevice.TYPE_ID,
  [SpiBusDevice.TYPE_NAME]: SpiBusDevice.TYPE_ID,
  [Ssd1306Device.TYPE_NAME]: Ssd1306Device.TYPE_ID,
  [St7735Device.TYPE_NAME]: St7735Device.TYPE_ID,
  [RtcDs3231Device.TYPE_NAME]: RtcDs3231Device.TYPE_ID,
  [Pcf8574ExpanderDevice.TYPE_NAME]: Pcf8574ExpanderDevice.TYPE_ID,
  [Pcf8575ExpanderDevice.TYPE_NAME]: Pcf8575ExpanderDevice.TYPE_ID,
  [PortExpanderSwitchDevice.TYPE_NAME]: PortExpanderSwitchDevice.TYPE_ID,
  [ScheduleDevice.TYPE_NAME]: ScheduleDevice.TYPE_ID,
  [AutoSwitchDevice.TYPE_NAME]: AutoSwitchDevice.TYPE_ID,
  [BinarySensorDevice.TYPE_NAME]: BinarySensorDevice.TYPE_ID,
  [DosingPumpDevice.TYPE_NAME]: DosingPumpDevice.TYPE_ID,
  [AnalogOutputDevice.TYPE_NAME]: AnalogOutputDevice.TYPE_ID,
  [FadeAnalogOutputDevice.TYPE_NAME]: FadeAnalogOutputDevice.TYPE_ID,
  [ScheduledAnalogOutputDevice.TYPE_NAME]: ScheduledAnalogOutputDevice.TYPE_ID,
  [AnalogOutputComposerDevice.TYPE_NAME]: AnalogOutputComposerDevice.TYPE_ID,
}

const deviceTypeNames: Record<DeviceTypeId, DeviceTypeName> = {
  [DummyDevice.TYPE_ID]: DummyDevice.TYPE_NAME,
  [GpioSwitchDevice.TYPE_ID]: GpioSwitchDevice.TYPE_NAME,
  [OneWireBusDevice.TYPE_ID]: OneWireBusDevice.TYPE_NAME,
  [I2cBusDevice.TYPE_ID]: I2cBusDevice.TYPE_NAME,
  [Ds18b20Device.TYPE_ID]: Ds18b20Device.TYPE_NAME,
  [NtcThermistorDevice.TYPE_ID]: NtcThermistorDevice.TYPE_NAME,
  [Htu21Device.TYPE_ID]: Htu21Device.TYPE_NAME,
  [ThermostatDevice.TYPE_ID]: ThermostatDevice.TYPE_NAME,
  [SpiBusDevice.TYPE_ID]: SpiBusDevice.TYPE_NAME,
  [Ssd1306Device.TYPE_ID]: Ssd1306Device.TYPE_NAME,
  [St7735Device.TYPE_ID]: St7735Device.TYPE_NAME,
  [RtcDs3231Device.TYPE_ID]: RtcDs3231Device.TYPE_NAME,
  [Pcf8574ExpanderDevice.TYPE_ID]: Pcf8574ExpanderDevice.TYPE_NAME,
  [Pcf8575ExpanderDevice.TYPE_ID]: Pcf8575ExpanderDevice.TYPE_NAME,
  [PortExpanderSwitchDevice.TYPE_ID]: PortExpanderSwitchDevice.TYPE_NAME,
  [ScheduleDevice.TYPE_ID]: ScheduleDevice.TYPE_NAME,
  [AutoSwitchDevice.TYPE_ID]: AutoSwitchDevice.TYPE_NAME,
  [BinarySensorDevice.TYPE_ID]: BinarySensorDevice.TYPE_NAME,
  [DosingPumpDevice.TYPE_ID]: DosingPumpDevice.TYPE_NAME,
  [AnalogOutputDevice.TYPE_ID]: AnalogOutputDevice.TYPE_NAME,
  [FadeAnalogOutputDevice.TYPE_ID]: FadeAnalogOutputDevice.TYPE_NAME,
  [ScheduledAnalogOutputDevice.TYPE_ID]: ScheduledAnalogOutputDevice.TYPE_NAME,
  [AnalogOutputComposerDevice.TYPE_ID]: AnalogOutputComposerDevice.TYPE_NAME,
}

// Mirrors the firmware's DeviceDependencyRole wire names (kDeviceDependencyRoleNames in
// src/devices/core/DeviceTypes.cpp) so picker filtering on the frontend uses the same role
// vocabulary devices already exchange via their `deps` links.
export type DeviceRole =
  | 'unknown'
  | 'onewire_bus'
  | 'temperature_sensor'
  | 'switch'
  | 'i2c_bus'
  | 'ssd1306'
  | 'spi_bus'
  | 'metric_source'
  | 'real_time_clock'
  | 'port_expander'
  | 'schedule'
  | 'condition'
  | 'analog_output'
  | 'analog_output_group'

export function deviceTypeIdFromName(typeName: string | undefined | null): DeviceTypeId {
  return typeName && typeName in deviceTypeIds ? deviceTypeIds[typeName as DeviceTypeName] : 0
}

export function deviceTypeName(typeId: number): DeviceTypeName {
  return deviceTypeNames[typeId] ?? DummyDevice.TYPE_NAME
}
