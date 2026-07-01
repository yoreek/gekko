import type { DeviceUiV2 } from './device-ui-types'
import { deviceTypeIdFromName } from '@/models/device-type-ids'
import { DummyDevice } from '@/models/devices/dummy'
import { GpioSwitchDevice } from '@/models/devices/gpio-switch'
import DummyFields from '@/v2/components/devices/dummy/DummyFields.vue'
import DummyWidget from '@/v2/components/devices/dummy/DummyWidget.vue'
import GpioSwitchFields from '@/v2/components/devices/gpio-switch/GpioSwitchFields.vue'
import GpioSwitchWidget from '@/v2/components/devices/gpio-switch/GpioSwitchWidget.vue'

const unknownUi: DeviceUiV2 = {
  typeId: 0,
  typeName: '',
  labelKey: 'device.type.unknown',
  icon: 'device',
  fieldsComponent: DummyFields,
  widgetComponent: DummyWidget,
}

const dummyUi: DeviceUiV2 = {
  typeId: DummyDevice.TYPE_ID,
  typeName: DummyDevice.TYPE_NAME,
  labelKey: 'device.type.dummy',
  icon: 'device',
  fieldsComponent: DummyFields,
  widgetComponent: DummyWidget,
}

const gpioSwitchUi: DeviceUiV2 = {
  typeId: GpioSwitchDevice.TYPE_ID,
  typeName: GpioSwitchDevice.TYPE_NAME,
  labelKey: 'device.type.gpioSwitch',
  icon: 'power',
  fieldsComponent: GpioSwitchFields,
  widgetComponent: GpioSwitchWidget,
}

const deviceUiV2ByTypeId: Record<number, DeviceUiV2> = {
  [dummyUi.typeId]: dummyUi,
  [gpioSwitchUi.typeId]: gpioSwitchUi,
}

export const allDeviceUisV2: DeviceUiV2[] = Object.values(deviceUiV2ByTypeId)

export function resolveDeviceUiV2ByTypeId(typeId: number): DeviceUiV2 {
  return deviceUiV2ByTypeId[typeId] ?? unknownUi
}

export function resolveDeviceUiV2(typeIdOrName: number | string | undefined | null): DeviceUiV2 {
  return typeof typeIdOrName === 'number'
    ? resolveDeviceUiV2ByTypeId(typeIdOrName)
    : resolveDeviceUiV2ByTypeId(deviceTypeIdFromName(typeIdOrName))
}
