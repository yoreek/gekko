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
import { Htu21Device } from '@/models/devices/htu21'
import { ThermostatDevice } from '@/models/devices/thermostat'
import { Ssd1306Device } from '@/models/devices/ssd1306/device'
import { St7735Device } from '@/models/devices/st7735/device'
import { RtcDs3231Device } from '@/models/devices/rtc-ds3231'
import { Pcf8574ExpanderDevice } from '@/models/devices/pcf8574-expander'
import { Pcf8575ExpanderDevice } from '@/models/devices/pcf8575-expander'
import { PortExpanderSwitchDevice } from '@/models/devices/port-expander-switch'
import { ScheduleDevice } from '@/models/devices/schedule'
import { AutoSwitchDevice } from '@/models/devices/auto-switch'
import { BinarySensorDevice } from '@/models/devices/binary-sensor'
import { DosingPumpDevice } from '@/models/devices/dosing-pump'
import { AnalogOutputDevice } from '@/models/devices/analog-output'
import { AnalogOutputComposerDevice, FadeAnalogOutputDevice, ScheduledAnalogOutputDevice } from '@/models/devices/composable-analog-output'
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
import Htu21Fields from '@/components/devices/htu21/Htu21Fields.vue'
import Htu21Widget from '@/components/devices/htu21/Htu21Widget.vue'
import ThermostatFields from '@/components/devices/thermostat/ThermostatFields.vue'
import ThermostatWidget from '@/components/devices/thermostat/ThermostatWidget.vue'
import Ssd1306Fields from '@/components/devices/Ssd1306Fields.vue'
import St7735Fields from '@/components/devices/St7735Fields.vue'
import RtcDs3231Fields from '@/components/devices/rtc-ds3231/RtcDs3231Fields.vue'
import RtcDs3231Widget from '@/components/devices/rtc-ds3231/RtcDs3231Widget.vue'
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
  icon: 'bus-onewire',
  fieldsComponent: OneWireBusFields,
  widgetComponent: OneWireBusWidget,
}

const i2cBusUi: DeviceUi = {
  typeId: I2cBusDevice.TYPE_ID,
  typeName: I2cBusDevice.TYPE_NAME,
  labelKey: 'device.type.i2cBus',
  icon: 'bus-i2c',
  fieldsComponent: I2cBusFields,
  widgetComponent: I2cBusWidget,
}

const spiBusUi: DeviceUi = {
  typeId: SpiBusDevice.TYPE_ID,
  typeName: SpiBusDevice.TYPE_NAME,
  labelKey: 'device.type.spiBus',
  icon: 'bus-spi',
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

const ntcThermistorUi: DeviceUi = {
  typeId: NtcThermistorDevice.TYPE_ID,
  typeName: NtcThermistorDevice.TYPE_NAME,
  labelKey: 'device.type.ntcThermistorTemperatureSensor',
  icon: 'temperature',
  fieldsComponent: NtcThermistorFields,
  widgetComponent: NtcThermistorWidget,
}

const htu21Ui: DeviceUi = {
  typeId: Htu21Device.TYPE_ID,
  typeName: Htu21Device.TYPE_NAME,
  labelKey: 'device.type.htu21',
  icon: 'temperature',
  fieldsComponent: Htu21Fields,
  widgetComponent: Htu21Widget,
}

const thermostatUi: DeviceUi = {
  typeId: ThermostatDevice.TYPE_ID,
  typeName: ThermostatDevice.TYPE_NAME,
  labelKey: 'device.type.thermostat',
  icon: 'thermostat',
  fieldsComponent: ThermostatFields,
  widgetComponent: ThermostatWidget,
}

const ssd1306Ui: DeviceUi = {
  typeId: Ssd1306Device.TYPE_ID,
  typeName: Ssd1306Device.TYPE_NAME,
  labelKey: 'device.type.ssd1306Display',
  icon: 'display',
  fieldsComponent: Ssd1306Fields,
  widgetComponent: DummyWidget,
}

const st7735Ui: DeviceUi = {
  typeId: St7735Device.TYPE_ID,
  typeName: St7735Device.TYPE_NAME,
  labelKey: 'device.type.st7735',
  icon: 'display',
  fieldsComponent: St7735Fields,
  widgetComponent: DummyWidget,
}

const rtcDs3231Ui: DeviceUi = {
  typeId: RtcDs3231Device.TYPE_ID,
  typeName: RtcDs3231Device.TYPE_NAME,
  labelKey: 'device.type.rtcDs3231',
  icon: 'time',
  fieldsComponent: RtcDs3231Fields,
  widgetComponent: RtcDs3231Widget,
}

const pcf8574ExpanderUi: DeviceUi = {
  typeId: Pcf8574ExpanderDevice.TYPE_ID,
  typeName: Pcf8574ExpanderDevice.TYPE_NAME,
  labelKey: 'device.type.pcf8574Expander',
  icon: 'chip',
  fieldsComponent: Pcf857xExpanderFields,
  widgetComponent: Pcf857xExpanderWidget,
}

const pcf8575ExpanderUi: DeviceUi = {
  typeId: Pcf8575ExpanderDevice.TYPE_ID,
  typeName: Pcf8575ExpanderDevice.TYPE_NAME,
  labelKey: 'device.type.pcf8575Expander',
  icon: 'chip',
  fieldsComponent: Pcf857xExpanderFields,
  widgetComponent: Pcf857xExpanderWidget,
}

const portExpanderSwitchUi: DeviceUi = {
  typeId: PortExpanderSwitchDevice.TYPE_ID,
  typeName: PortExpanderSwitchDevice.TYPE_NAME,
  labelKey: 'device.type.portExpanderSwitch',
  icon: 'power',
  fieldsComponent: PortExpanderSwitchFields,
  widgetComponent: PortExpanderSwitchWidget,
}

const scheduleUi: DeviceUi = {
  typeId: ScheduleDevice.TYPE_ID,
  typeName: ScheduleDevice.TYPE_NAME,
  labelKey: 'device.type.schedule',
  icon: 'time',
  fieldsComponent: ScheduleFields,
  widgetComponent: ScheduleWidget,
}

const autoSwitchUi: DeviceUi = {
  typeId: AutoSwitchDevice.TYPE_ID,
  typeName: AutoSwitchDevice.TYPE_NAME,
  labelKey: 'device.type.autoSwitch',
  icon: 'power',
  fieldsComponent: AutoSwitchFields,
  widgetComponent: AutoSwitchWidget,
}

const binarySensorUi: DeviceUi = {
  typeId: BinarySensorDevice.TYPE_ID,
  typeName: BinarySensorDevice.TYPE_NAME,
  labelKey: 'device.type.binarySensor',
  icon: 'check-circle',
  fieldsComponent: BinarySensorFields,
  widgetComponent: BinarySensorWidget,
}

const dosingPumpUi: DeviceUi = {
  typeId: DosingPumpDevice.TYPE_ID,
  typeName: DosingPumpDevice.TYPE_NAME,
  labelKey: 'device.type.dosingPump',
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
  icon: 'analog-output',
  fieldsComponent: AnalogOutputFields,
  widgetComponent: AnalogOutputWidget,
}

const fadeAnalogOutputUi: DeviceUi = {
  typeId: FadeAnalogOutputDevice.TYPE_ID,
  typeName: FadeAnalogOutputDevice.TYPE_NAME,
  labelKey: 'device.type.fadeAnalogOutput',
  icon: 'analog-output',
  fieldsComponent: FadeAnalogOutputFields,
  widgetComponent: AnalogOutputWidget,
}

const scheduledAnalogOutputUi: DeviceUi = {
  typeId: ScheduledAnalogOutputDevice.TYPE_ID,
  typeName: ScheduledAnalogOutputDevice.TYPE_NAME,
  labelKey: 'device.type.scheduledAnalogOutput',
  icon: 'analog-output',
  fieldsComponent: ScheduledAnalogOutputFields,
  widgetComponent: ScheduledAnalogOutputWidget,
  moreInfoMaxWidth: 720,
}

const analogOutputComposerUi: DeviceUi = {
  typeId: AnalogOutputComposerDevice.TYPE_ID,
  typeName: AnalogOutputComposerDevice.TYPE_NAME,
  labelKey: 'device.type.analogOutputComposer',
  icon: 'analog-output',
  fieldsComponent: AnalogOutputComposerFields,
  widgetComponent: AnalogOutputComposerWidget,
  moreInfoMaxWidth: 960,
}

const deviceUiV2ByTypeId: Record<number, DeviceUi> = {
  [dummyUi.typeId]: dummyUi,
  [gpioSwitchUi.typeId]: gpioSwitchUi,
  [oneWireBusUi.typeId]: oneWireBusUi,
  [i2cBusUi.typeId]: i2cBusUi,
  [spiBusUi.typeId]: spiBusUi,
  [ds18b20Ui.typeId]: ds18b20Ui,
  [ntcThermistorUi.typeId]: ntcThermistorUi,
  [htu21Ui.typeId]: htu21Ui,
  [thermostatUi.typeId]: thermostatUi,
  [ssd1306Ui.typeId]: ssd1306Ui,
  [st7735Ui.typeId]: st7735Ui,
  [rtcDs3231Ui.typeId]: rtcDs3231Ui,
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
