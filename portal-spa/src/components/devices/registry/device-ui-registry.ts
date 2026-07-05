import type { DeviceUi } from './device-ui-types'
import { deviceTypeIdFromName } from '@/models/device-type-ids'
import { DummyDevice } from '@/models/devices/dummy'
import { GpioSwitchDevice } from '@/models/devices/gpio-switch'
import { OneWireBusDevice } from '@/models/devices/onewire-bus'
import { I2cBusDevice } from '@/models/devices/i2c-bus'
import { SpiBusDevice } from '@/models/devices/spi-bus'
import { Ds18b20Device } from '@/models/devices/ds18b20'
import { ThermostatDevice } from '@/models/devices/thermostat'
import { Ssd1306Device } from '@/models/devices/ssd1306/device'
import { St7735Device } from '@/models/devices/st7735/device'
import DummyFields from '@/components/devices/dummy/DummyFields.vue'
import DummyWidget from '@/components/devices/dummy/DummyWidget.vue'
import GpioSwitchFields from '@/components/devices/gpio-switch/GpioSwitchFields.vue'
import GpioSwitchWidget from '@/components/devices/gpio-switch/GpioSwitchWidget.vue'
import OneWireBusFields from '@/components/devices/onewire-bus/OneWireBusFields.vue'
import OneWireBusWidget from '@/components/devices/onewire-bus/OneWireBusWidget.vue'
import I2cBusFields from '@/components/devices/i2c-bus/I2cBusFields.vue'
import I2cBusWidget from '@/components/devices/i2c-bus/I2cBusWidget.vue'
import SpiBusFields from '@/components/devices/spi-bus/SpiBusFields.vue'
import SpiBusWidget from '@/components/devices/spi-bus/SpiBusWidget.vue'
import Ds18b20Fields from '@/components/devices/ds18b20/Ds18b20Fields.vue'
import Ds18b20Widget from '@/components/devices/ds18b20/Ds18b20Widget.vue'
import ThermostatFields from '@/components/devices/thermostat/ThermostatFields.vue'
import ThermostatWidget from '@/components/devices/thermostat/ThermostatWidget.vue'
import Ssd1306Fields from '@/components/devices/Ssd1306Fields.vue'
import St7735Fields from '@/components/devices/St7735Fields.vue'

const unknownUi: DeviceUi = {
  typeId: 0,
  typeName: '',
  labelKey: 'device.type.unknown',
  icon: 'device',
  fieldsComponent: DummyFields,
  widgetComponent: DummyWidget,
}

const dummyUi: DeviceUi = {
  typeId: DummyDevice.TYPE_ID,
  typeName: DummyDevice.TYPE_NAME,
  labelKey: 'device.type.dummy',
  icon: 'device',
  fieldsComponent: DummyFields,
  widgetComponent: DummyWidget,
}

const gpioSwitchUi: DeviceUi = {
  typeId: GpioSwitchDevice.TYPE_ID,
  typeName: GpioSwitchDevice.TYPE_NAME,
  labelKey: 'device.type.gpioSwitch',
  icon: 'power',
  fieldsComponent: GpioSwitchFields,
  widgetComponent: GpioSwitchWidget,
}

const oneWireBusUi: DeviceUi = {
  typeId: OneWireBusDevice.TYPE_ID,
  typeName: OneWireBusDevice.TYPE_NAME,
  labelKey: 'device.type.onewireBus',
  icon: 'bus',
  fieldsComponent: OneWireBusFields,
  widgetComponent: OneWireBusWidget,
}

const i2cBusUi: DeviceUi = {
  typeId: I2cBusDevice.TYPE_ID,
  typeName: I2cBusDevice.TYPE_NAME,
  labelKey: 'device.type.i2cBus',
  icon: 'bus',
  fieldsComponent: I2cBusFields,
  widgetComponent: I2cBusWidget,
}

const spiBusUi: DeviceUi = {
  typeId: SpiBusDevice.TYPE_ID,
  typeName: SpiBusDevice.TYPE_NAME,
  labelKey: 'device.type.spiBus',
  icon: 'bus',
  fieldsComponent: SpiBusFields,
  widgetComponent: SpiBusWidget,
}

const ds18b20Ui: DeviceUi = {
  typeId: Ds18b20Device.TYPE_ID,
  typeName: Ds18b20Device.TYPE_NAME,
  labelKey: 'device.type.ds18b20TemperatureSensor',
  icon: 'temperature',
  fieldsComponent: Ds18b20Fields,
  widgetComponent: Ds18b20Widget,
}

const thermostatUi: DeviceUi = {
  typeId: ThermostatDevice.TYPE_ID,
  typeName: ThermostatDevice.TYPE_NAME,
  labelKey: 'device.type.thermostat',
  icon: 'temperature',
  fieldsComponent: ThermostatFields,
  widgetComponent: ThermostatWidget,
}

const ssd1306Ui: DeviceUi = {
  typeId: Ssd1306Device.TYPE_ID,
  typeName: Ssd1306Device.TYPE_NAME,
  labelKey: 'device.type.ssd1306Display',
  icon: 'device',
  fieldsComponent: Ssd1306Fields,
  widgetComponent: DummyWidget,
}

const st7735Ui: DeviceUi = {
  typeId: St7735Device.TYPE_ID,
  typeName: St7735Device.TYPE_NAME,
  labelKey: 'device.type.st7735',
  icon: 'device',
  fieldsComponent: St7735Fields,
  widgetComponent: DummyWidget,
}

const deviceUiV2ByTypeId: Record<number, DeviceUi> = {
  [dummyUi.typeId]: dummyUi,
  [gpioSwitchUi.typeId]: gpioSwitchUi,
  [oneWireBusUi.typeId]: oneWireBusUi,
  [i2cBusUi.typeId]: i2cBusUi,
  [spiBusUi.typeId]: spiBusUi,
  [ds18b20Ui.typeId]: ds18b20Ui,
  [thermostatUi.typeId]: thermostatUi,
  [ssd1306Ui.typeId]: ssd1306Ui,
  [st7735Ui.typeId]: st7735Ui,
}

export const allDeviceUis: DeviceUi[] = Object.values(deviceUiV2ByTypeId)

export function resolveDeviceUiByTypeId(typeId: number): DeviceUi {
  return deviceUiV2ByTypeId[typeId] ?? unknownUi
}

export function resolveDeviceUi(typeIdOrName: number | string | undefined | null): DeviceUi {
  return typeof typeIdOrName === 'number'
    ? resolveDeviceUiByTypeId(typeIdOrName)
    : resolveDeviceUiByTypeId(deviceTypeIdFromName(typeIdOrName))
}
