import type { DeviceUi } from './device-ui-types'
import type { DosingPumpOutputSnapshot } from '@/api/contracts'
import { deviceTypeIdFromName } from '@/models/device-type-ids'
import { DummyDevice } from '@/models/devices/dummy'
import { GpioSwitchDevice } from '@/models/devices/gpio-switch'
import { OneWireBusDevice } from '@/models/devices/onewire-bus'
import { I2cBusDevice } from '@/models/devices/i2c-bus'
import { SpiBusDevice } from '@/models/devices/spi-bus'
import { Ds18b20Device } from '@/models/devices/ds18b20'
import { NtcThermistorDevice } from '@/models/devices/ntc-thermistor'
import { Aht10Device } from '@/models/devices/aht10'
import { Dht11Device } from '@/models/devices/dht11'
import { Htu21Device } from '@/models/devices/htu21'
import { ThermostatDevice } from '@/models/devices/thermostat'
import { Ssd1306Device } from '@/models/devices/ssd1306/device'
import { St7735Device } from '@/models/devices/st7735/device'
import { RtcDs3231Device } from '@/models/devices/rtc-ds3231'
import { RtcDs1302Device } from '@/models/devices/rtc-ds1302'
import { Pcf8574ExpanderDevice } from '@/models/devices/pcf8574-expander'
import { Pcf8575ExpanderDevice } from '@/models/devices/pcf8575-expander'
import { PortExpanderSwitchDevice } from '@/models/devices/port-expander-switch'
import { ScheduleDevice } from '@/models/devices/schedule'
import { AutoSwitchDevice } from '@/models/devices/auto-switch'
import { BinarySensorDevice } from '@/models/devices/binary-sensor'
import { DosingPumpDevice } from '@/models/devices/dosing-pump'
import { AnalogOutputDevice } from '@/models/devices/analog-output'
import { AnalogOutputComposerDevice, FadeAnalogOutputDevice, ScheduledAnalogOutputDevice } from '@/models/devices/composable-analog-output'
import { AnalogPortInputDevice } from '@/models/devices/analog-port-input'
import { Ads1115HubDevice } from '@/models/devices/ads1115-hub'
import { AnalogInputChannelDevice } from '@/models/devices/analog-input-channel'
import { Cd74hc4067HubDevice } from '@/models/devices/cd74hc4067-hub'
import { Lcd1602Device } from '@/models/devices/lcd1602'
import { Lcd2004Device } from '@/models/devices/lcd2004'
import { Lcd1602PinDevice } from '@/models/devices/lcd1602-pin'
import { Lcd2004PinDevice } from '@/models/devices/lcd2004-pin'
import { Tm1637Device } from '@/models/devices/tm1637'
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
import NtcThermistorFields from '@/components/devices/ntc-thermistor/NtcThermistorFields.vue'
import NtcThermistorWidget from '@/components/devices/ntc-thermistor/NtcThermistorWidget.vue'
import Aht10Fields from '@/components/devices/aht10/Aht10Fields.vue'
import Aht10Widget from '@/components/devices/aht10/Aht10Widget.vue'
import Dht11Fields from '@/components/devices/dht11/Dht11Fields.vue'
import Dht11Widget from '@/components/devices/dht11/Dht11Widget.vue'
import Htu21Fields from '@/components/devices/htu21/Htu21Fields.vue'
import Htu21Widget from '@/components/devices/htu21/Htu21Widget.vue'
import ThermostatFields from '@/components/devices/thermostat/ThermostatFields.vue'
import ThermostatWidget from '@/components/devices/thermostat/ThermostatWidget.vue'
import Ssd1306Fields from '@/components/devices/Ssd1306Fields.vue'
import St7735Fields from '@/components/devices/St7735Fields.vue'
import Ssd1306Widget from '@/components/devices/ssd1306/Ssd1306Widget.vue'
import St7735Widget from '@/components/devices/st7735/St7735Widget.vue'
import RtcDs3231Fields from '@/components/devices/rtc-ds3231/RtcDs3231Fields.vue'
import RtcDs3231Widget from '@/components/devices/rtc-ds3231/RtcDs3231Widget.vue'
import RtcDs1302Fields from '@/components/devices/rtc-ds1302/RtcDs1302Fields.vue'
import RtcDs1302Widget from '@/components/devices/rtc-ds1302/RtcDs1302Widget.vue'
import Pcf857xExpanderFields from '@/components/devices/expander/Pcf857xExpanderFields.vue'
import Pcf857xExpanderWidget from '@/components/devices/expander/Pcf857xExpanderWidget.vue'
import PortExpanderSwitchFields from '@/components/devices/port-expander-switch/PortExpanderSwitchFields.vue'
import PortExpanderSwitchWidget from '@/components/devices/port-expander-switch/PortExpanderSwitchWidget.vue'
import ScheduleFields from '@/components/devices/schedule/ScheduleFields.vue'
import ScheduleWidget from '@/components/devices/schedule/ScheduleWidget.vue'
import AutoSwitchFields from '@/components/devices/auto-switch/AutoSwitchFields.vue'
import AutoSwitchWidget from '@/components/devices/auto-switch/AutoSwitchWidget.vue'
import BinarySensorFields from '@/components/devices/binary-sensor/BinarySensorFields.vue'
import BinarySensorWidget from '@/components/devices/binary-sensor/BinarySensorWidget.vue'
import DosingPumpFields from '@/components/devices/dosing-pump/DosingPumpFields.vue'
import DosingPumpWidget from '@/components/devices/dosing-pump/DosingPumpWidget.vue'
import AnalogOutputFields from '@/components/devices/analog-output/AnalogOutputFields.vue'
import AnalogOutputWidget from '@/components/devices/analog-output/AnalogOutputWidget.vue'
import FadeAnalogOutputFields from '@/components/devices/analog-output/FadeAnalogOutputFields.vue'
import ScheduledAnalogOutputFields from '@/components/devices/analog-output/ScheduledAnalogOutputFields.vue'
import ScheduledAnalogOutputWidget from '@/components/devices/analog-output/ScheduledAnalogOutputWidget.vue'
import AnalogOutputComposerFields from '@/components/devices/analog-output/AnalogOutputComposerFields.vue'
import AnalogOutputComposerWidget from '@/components/devices/analog-output/AnalogOutputComposerWidget.vue'
import AnalogPortInputFields from '@/components/devices/analog-port-input/AnalogPortInputFields.vue'
import AnalogPortInputWidget from '@/components/devices/analog-port-input/AnalogPortInputWidget.vue'
import Ads1115HubFields from '@/components/devices/ads1115-hub/Ads1115HubFields.vue'
import Cd74hc4067HubFields from '@/components/devices/cd74hc4067-hub/Cd74hc4067HubFields.vue'
import AnalogInputHubWidget from '@/components/devices/analog-input/AnalogInputHubWidget.vue'
import AnalogInputChannelFields from '@/components/devices/analog-input/AnalogInputChannelFields.vue'
import AnalogInputChannelWidget from '@/components/devices/analog-input/AnalogInputChannelWidget.vue'
import Lcd1602Fields from '@/components/devices/lcd1602/Lcd1602Fields.vue'
import Lcd1602Widget from '@/components/devices/lcd1602/Lcd1602Widget.vue'
import Lcd2004Fields from '@/components/devices/lcd2004/Lcd2004Fields.vue'
import Lcd2004Widget from '@/components/devices/lcd2004/Lcd2004Widget.vue'
import Lcd1602PinFields from '@/components/devices/lcd1602-pin/Lcd1602PinFields.vue'
import Lcd1602PinWidget from '@/components/devices/lcd1602-pin/Lcd1602PinWidget.vue'
import Lcd2004PinFields from '@/components/devices/lcd2004-pin/Lcd2004PinFields.vue'
import Lcd2004PinWidget from '@/components/devices/lcd2004-pin/Lcd2004PinWidget.vue'
import Tm1637Fields from '@/components/devices/tm1637/Tm1637Fields.vue'
import Tm1637Widget from '@/components/devices/tm1637/Tm1637Widget.vue'
import DisplayDesignerView from '@/views/DisplayDesignerView.vue'

const unknownUi: DeviceUi = {
  typeId: 0,
  typeName: '',
  labelKey: 'device.type.unknown',
  category: 'service',
  icon: 'device',
  fieldsComponent: DummyFields,
  widgetComponent: DummyWidget,
}

const dummyUi: DeviceUi = {
  typeId: DummyDevice.TYPE_ID,
  typeName: DummyDevice.TYPE_NAME,
  labelKey: 'device.type.dummy',
  category: 'service',
  icon: 'device',
  fieldsComponent: DummyFields,
  widgetComponent: DummyWidget,
}

const gpioSwitchUi: DeviceUi = {
  typeId: GpioSwitchDevice.TYPE_ID,
  typeName: GpioSwitchDevice.TYPE_NAME,
  labelKey: 'device.type.gpioSwitch',
  category: 'outputs',
  icon: 'power',
  fieldsComponent: GpioSwitchFields,
  widgetComponent: GpioSwitchWidget,
}

const oneWireBusUi: DeviceUi = {
  typeId: OneWireBusDevice.TYPE_ID,
  typeName: OneWireBusDevice.TYPE_NAME,
  labelKey: 'device.type.onewireBus',
  category: 'buses',
  searchAliases: ['1-wire'],
  icon: 'bus-onewire',
  fieldsComponent: OneWireBusFields,
  widgetComponent: OneWireBusWidget,
}

const i2cBusUi: DeviceUi = {
  typeId: I2cBusDevice.TYPE_ID,
  typeName: I2cBusDevice.TYPE_NAME,
  labelKey: 'device.type.i2cBus',
  category: 'buses',
  icon: 'bus-i2c',
  fieldsComponent: I2cBusFields,
  widgetComponent: I2cBusWidget,
}

const spiBusUi: DeviceUi = {
  typeId: SpiBusDevice.TYPE_ID,
  typeName: SpiBusDevice.TYPE_NAME,
  labelKey: 'device.type.spiBus',
  category: 'buses',
  icon: 'bus-spi',
  fieldsComponent: SpiBusFields,
  widgetComponent: SpiBusWidget,
}

const ds18b20Ui: DeviceUi = {
  typeId: Ds18b20Device.TYPE_ID,
  typeName: Ds18b20Device.TYPE_NAME,
  labelKey: 'device.type.ds18b20TemperatureSensor',
  category: 'temperatureSensors',
  icon: 'temperature',
  fieldsComponent: Ds18b20Fields,
  widgetComponent: Ds18b20Widget,
}

const ntcThermistorUi: DeviceUi = {
  typeId: NtcThermistorDevice.TYPE_ID,
  typeName: NtcThermistorDevice.TYPE_NAME,
  labelKey: 'device.type.ntcThermistorTemperatureSensor',
  category: 'temperatureSensors',
  icon: 'temperature',
  fieldsComponent: NtcThermistorFields,
  widgetComponent: NtcThermistorWidget,
}

const aht10Ui: DeviceUi = {
  typeId: Aht10Device.TYPE_ID,
  typeName: Aht10Device.TYPE_NAME,
  labelKey: 'device.type.aht10',
  category: 'temperatureSensors',
  icon: 'temperature',
  fieldsComponent: Aht10Fields,
  widgetComponent: Aht10Widget,
}

const dht11Ui: DeviceUi = {
  typeId: Dht11Device.TYPE_ID,
  typeName: Dht11Device.TYPE_NAME,
  labelKey: 'device.type.dht11',
  category: 'temperatureSensors',
  icon: 'temperature',
  fieldsComponent: Dht11Fields,
  widgetComponent: Dht11Widget,
}

const htu21Ui: DeviceUi = {
  typeId: Htu21Device.TYPE_ID,
  typeName: Htu21Device.TYPE_NAME,
  labelKey: 'device.type.htu21',
  category: 'temperatureSensors',
  icon: 'temperature',
  fieldsComponent: Htu21Fields,
  widgetComponent: Htu21Widget,
}

const thermostatUi: DeviceUi = {
  typeId: ThermostatDevice.TYPE_ID,
  typeName: ThermostatDevice.TYPE_NAME,
  labelKey: 'device.type.thermostat',
  category: 'controllers',
  icon: 'thermostat',
  fieldsComponent: ThermostatFields,
  widgetComponent: ThermostatWidget,
}

const ssd1306Ui: DeviceUi = {
  typeId: Ssd1306Device.TYPE_ID,
  typeName: Ssd1306Device.TYPE_NAME,
  labelKey: 'device.type.ssd1306Display',
  category: 'displays',
  icon: 'display',
  fieldsComponent: Ssd1306Fields,
  widgetComponent: Ssd1306Widget,
  moreInfoMaxWidth: 720,
  designerComponent: DisplayDesignerView,
}

const st7735Ui: DeviceUi = {
  typeId: St7735Device.TYPE_ID,
  typeName: St7735Device.TYPE_NAME,
  labelKey: 'device.type.st7735',
  category: 'displays',
  icon: 'display',
  fieldsComponent: St7735Fields,
  widgetComponent: St7735Widget,
  moreInfoMaxWidth: 720,
  designerComponent: DisplayDesignerView,
}

const rtcDs3231Ui: DeviceUi = {
  typeId: RtcDs3231Device.TYPE_ID,
  typeName: RtcDs3231Device.TYPE_NAME,
  labelKey: 'device.type.rtcDs3231',
  category: 'rtc',
  searchAliases: ['real time clock'],
  icon: 'time',
  fieldsComponent: RtcDs3231Fields,
  widgetComponent: RtcDs3231Widget,
}

const rtcDs1302Ui: DeviceUi = {
  typeId: RtcDs1302Device.TYPE_ID,
  typeName: RtcDs1302Device.TYPE_NAME,
  labelKey: 'device.type.rtcDs1302',
  category: 'rtc',
  searchAliases: ['real time clock'],
  icon: 'time',
  fieldsComponent: RtcDs1302Fields,
  widgetComponent: RtcDs1302Widget,
}

const pcf8574ExpanderUi: DeviceUi = {
  typeId: Pcf8574ExpanderDevice.TYPE_ID,
  typeName: Pcf8574ExpanderDevice.TYPE_NAME,
  labelKey: 'device.type.pcf8574Expander',
  category: 'expanders',
  icon: 'chip',
  fieldsComponent: Pcf857xExpanderFields,
  widgetComponent: Pcf857xExpanderWidget,
}

const pcf8575ExpanderUi: DeviceUi = {
  typeId: Pcf8575ExpanderDevice.TYPE_ID,
  typeName: Pcf8575ExpanderDevice.TYPE_NAME,
  labelKey: 'device.type.pcf8575Expander',
  category: 'expanders',
  icon: 'chip',
  fieldsComponent: Pcf857xExpanderFields,
  widgetComponent: Pcf857xExpanderWidget,
}

const portExpanderSwitchUi: DeviceUi = {
  typeId: PortExpanderSwitchDevice.TYPE_ID,
  typeName: PortExpanderSwitchDevice.TYPE_NAME,
  labelKey: 'device.type.portExpanderSwitch',
  category: 'outputs',
  icon: 'power',
  fieldsComponent: PortExpanderSwitchFields,
  widgetComponent: PortExpanderSwitchWidget,
}

const scheduleUi: DeviceUi = {
  typeId: ScheduleDevice.TYPE_ID,
  typeName: ScheduleDevice.TYPE_NAME,
  labelKey: 'device.type.schedule',
  category: 'automation',
  icon: 'time',
  fieldsComponent: ScheduleFields,
  widgetComponent: ScheduleWidget,
}

const autoSwitchUi: DeviceUi = {
  typeId: AutoSwitchDevice.TYPE_ID,
  typeName: AutoSwitchDevice.TYPE_NAME,
  labelKey: 'device.type.autoSwitch',
  category: 'automation',
  icon: 'power',
  fieldsComponent: AutoSwitchFields,
  widgetComponent: AutoSwitchWidget,
}

const binarySensorUi: DeviceUi = {
  typeId: BinarySensorDevice.TYPE_ID,
  typeName: BinarySensorDevice.TYPE_NAME,
  labelKey: 'device.type.binarySensor',
  category: 'inputs',
  icon: 'check-circle',
  fieldsComponent: BinarySensorFields,
  widgetComponent: BinarySensorWidget,
}

const dosingPumpUi: DeviceUi = {
  typeId: DosingPumpDevice.TYPE_ID,
  typeName: DosingPumpDevice.TYPE_NAME,
  labelKey: 'device.type.dosingPump',
  category: 'controllers',
  icon: 'pump',
  fieldsComponent: DosingPumpFields,
  widgetComponent: DosingPumpWidget,
  extractAlerts: device => {
    const output = (device.runtime as { output?: DosingPumpOutputSnapshot }).output
    const container = output?.container
    if (container?.status === 'critical') {
      return [{ kind: 'container', severity: 'critical', messageKey: 'notifications.alerts.containerEmpty' }]
    }
    if (container?.status === 'warning') {
      return [
        {
          kind: 'container',
          severity: 'warning',
          messageKey: 'notifications.alerts.containerLow',
          messageParams: { percent: Math.round(container.percent ?? 0) },
        },
      ]
    }
    return []
  },
}

const analogOutputUi: DeviceUi = {
  typeId: AnalogOutputDevice.TYPE_ID,
  typeName: AnalogOutputDevice.TYPE_NAME,
  labelKey: 'device.type.analogOutput',
  category: 'outputs',
  searchAliases: ['pwm'],
  icon: 'analog-output',
  fieldsComponent: AnalogOutputFields,
  widgetComponent: AnalogOutputWidget,
}

const fadeAnalogOutputUi: DeviceUi = {
  typeId: FadeAnalogOutputDevice.TYPE_ID,
  typeName: FadeAnalogOutputDevice.TYPE_NAME,
  labelKey: 'device.type.fadeAnalogOutput',
  category: 'outputs',
  searchAliases: ['pwm'],
  icon: 'analog-output',
  fieldsComponent: FadeAnalogOutputFields,
  widgetComponent: AnalogOutputWidget,
}

const scheduledAnalogOutputUi: DeviceUi = {
  typeId: ScheduledAnalogOutputDevice.TYPE_ID,
  typeName: ScheduledAnalogOutputDevice.TYPE_NAME,
  labelKey: 'device.type.scheduledAnalogOutput',
  category: 'outputs',
  searchAliases: ['pwm'],
  icon: 'analog-output',
  fieldsComponent: ScheduledAnalogOutputFields,
  widgetComponent: ScheduledAnalogOutputWidget,
  moreInfoMaxWidth: 720,
}

const analogOutputComposerUi: DeviceUi = {
  typeId: AnalogOutputComposerDevice.TYPE_ID,
  typeName: AnalogOutputComposerDevice.TYPE_NAME,
  labelKey: 'device.type.analogOutputComposer',
  category: 'outputs',
  searchAliases: ['pwm'],
  icon: 'analog-output',
  fieldsComponent: AnalogOutputComposerFields,
  widgetComponent: AnalogOutputComposerWidget,
  moreInfoMaxWidth: 960,
}

const analogPortInputUi: DeviceUi = {
  typeId: AnalogPortInputDevice.TYPE_ID,
  typeName: AnalogPortInputDevice.TYPE_NAME,
  labelKey: 'device.type.analogPortInput',
  category: 'inputs',
  searchAliases: ['adc'],
  icon: 'analog-input',
  fieldsComponent: AnalogPortInputFields,
  widgetComponent: AnalogPortInputWidget,
}

const ads1115HubUi: DeviceUi = {
  typeId: Ads1115HubDevice.TYPE_ID,
  typeName: Ads1115HubDevice.TYPE_NAME,
  labelKey: 'device.type.ads1115Hub',
  category: 'expanders',
  searchAliases: ['adc'],
  icon: 'chip',
  fieldsComponent: Ads1115HubFields,
  widgetComponent: AnalogInputHubWidget,
}

const cd74hc4067HubUi: DeviceUi = {
  typeId: Cd74hc4067HubDevice.TYPE_ID,
  typeName: Cd74hc4067HubDevice.TYPE_NAME,
  labelKey: 'device.type.cd74hc4067Hub',
  category: 'expanders',
  searchAliases: ['multiplexer', 'mux'],
  icon: 'chip',
  fieldsComponent: Cd74hc4067HubFields,
  widgetComponent: AnalogInputHubWidget,
}

const analogInputChannelUi: DeviceUi = {
  typeId: AnalogInputChannelDevice.TYPE_ID,
  typeName: AnalogInputChannelDevice.TYPE_NAME,
  labelKey: 'device.type.analogInputChannel',
  category: 'inputs',
  icon: 'analog-input',
  fieldsComponent: AnalogInputChannelFields,
  widgetComponent: AnalogInputChannelWidget,
}

const lcd1602Ui: DeviceUi = {
  typeId: Lcd1602Device.TYPE_ID,
  typeName: Lcd1602Device.TYPE_NAME,
  labelKey: 'device.type.lcd1602',
  category: 'displays',
  icon: 'display',
  fieldsComponent: Lcd1602Fields,
  widgetComponent: Lcd1602Widget,
  moreInfoMaxWidth: 720,
  designerComponent: DisplayDesignerView,
}

const lcd2004Ui: DeviceUi = {
  typeId: Lcd2004Device.TYPE_ID,
  typeName: Lcd2004Device.TYPE_NAME,
  labelKey: 'device.type.lcd2004',
  category: 'displays',
  icon: 'display',
  fieldsComponent: Lcd2004Fields,
  widgetComponent: Lcd2004Widget,
  moreInfoMaxWidth: 720,
  designerComponent: DisplayDesignerView,
}

const lcd1602PinUi: DeviceUi = {
  typeId: Lcd1602PinDevice.TYPE_ID,
  typeName: Lcd1602PinDevice.TYPE_NAME,
  labelKey: 'device.type.lcd1602Pin',
  category: 'displays',
  icon: 'display',
  fieldsComponent: Lcd1602PinFields,
  widgetComponent: Lcd1602PinWidget,
  moreInfoMaxWidth: 720,
  designerComponent: DisplayDesignerView,
}

const lcd2004PinUi: DeviceUi = {
  typeId: Lcd2004PinDevice.TYPE_ID,
  typeName: Lcd2004PinDevice.TYPE_NAME,
  labelKey: 'device.type.lcd2004Pin',
  category: 'displays',
  icon: 'display',
  fieldsComponent: Lcd2004PinFields,
  widgetComponent: Lcd2004PinWidget,
  moreInfoMaxWidth: 720,
  designerComponent: DisplayDesignerView,
}

const tm1637Ui: DeviceUi = {
  typeId: Tm1637Device.TYPE_ID,
  typeName: Tm1637Device.TYPE_NAME,
  labelKey: 'device.type.tm1637',
  category: 'displays',
  icon: 'display',
  fieldsComponent: Tm1637Fields,
  widgetComponent: Tm1637Widget,
  moreInfoMaxWidth: 720,
  designerComponent: DisplayDesignerView,
}

const deviceUiV2ByTypeId: Record<number, DeviceUi> = {
  [dummyUi.typeId]: dummyUi,
  [gpioSwitchUi.typeId]: gpioSwitchUi,
  [oneWireBusUi.typeId]: oneWireBusUi,
  [i2cBusUi.typeId]: i2cBusUi,
  [spiBusUi.typeId]: spiBusUi,
  [ds18b20Ui.typeId]: ds18b20Ui,
  [ntcThermistorUi.typeId]: ntcThermistorUi,
  [aht10Ui.typeId]: aht10Ui,
  [dht11Ui.typeId]: dht11Ui,
  [htu21Ui.typeId]: htu21Ui,
  [thermostatUi.typeId]: thermostatUi,
  [ssd1306Ui.typeId]: ssd1306Ui,
  [st7735Ui.typeId]: st7735Ui,
  [rtcDs3231Ui.typeId]: rtcDs3231Ui,
  [rtcDs1302Ui.typeId]: rtcDs1302Ui,
  [pcf8574ExpanderUi.typeId]: pcf8574ExpanderUi,
  [pcf8575ExpanderUi.typeId]: pcf8575ExpanderUi,
  [portExpanderSwitchUi.typeId]: portExpanderSwitchUi,
  [scheduleUi.typeId]: scheduleUi,
  [autoSwitchUi.typeId]: autoSwitchUi,
  [binarySensorUi.typeId]: binarySensorUi,
  [dosingPumpUi.typeId]: dosingPumpUi,
  [analogOutputUi.typeId]: analogOutputUi,
  [fadeAnalogOutputUi.typeId]: fadeAnalogOutputUi,
  [scheduledAnalogOutputUi.typeId]: scheduledAnalogOutputUi,
  [analogOutputComposerUi.typeId]: analogOutputComposerUi,
  [analogPortInputUi.typeId]: analogPortInputUi,
  [ads1115HubUi.typeId]: ads1115HubUi,
  [cd74hc4067HubUi.typeId]: cd74hc4067HubUi,
  [analogInputChannelUi.typeId]: analogInputChannelUi,
  [lcd1602Ui.typeId]: lcd1602Ui,
  [lcd2004Ui.typeId]: lcd2004Ui,
  [lcd1602PinUi.typeId]: lcd1602PinUi,
  [lcd2004PinUi.typeId]: lcd2004PinUi,
  [tm1637Ui.typeId]: tm1637Ui,
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
