import OneWireBusDeviceDetail from './OneWireBusDeviceDetail.vue'
import OneWireBusDeviceForm from './OneWireBusDeviceForm.vue'
import OneWireBusDeviceWidget from './OneWireBusDeviceWidget.vue'
import { OneWireBusDevice } from '@/models/devices/onewire-bus'
import type { DeviceUi } from '@/components/devices/registry/device-ui-types'

export const oneWireBusUi: DeviceUi = {
  typeId: OneWireBusDevice.TYPE_ID,
  typeName: OneWireBusDevice.TYPE_NAME,
  labelKey: 'device.type.onewireBus',
  icon: 'bus',
  detailComponent: OneWireBusDeviceDetail,
  formComponent: OneWireBusDeviceForm,
  widgetComponent: OneWireBusDeviceWidget,
}
