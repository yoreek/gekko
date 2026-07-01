import SpiBusDeviceDetail from './SpiBusDeviceDetail.vue'
import SpiBusDeviceForm from './SpiBusDeviceForm.vue'
import SpiBusDeviceEditor from './SpiBusDeviceEditor.vue'
import SpiBusDeviceWidget from './SpiBusDeviceWidget.vue'
import { SpiBusDevice } from '@/models/devices/spi-bus'
import type { DeviceUi } from '@/components/devices/registry/device-ui-types'

export const spiBusUi: DeviceUi = {
  typeId: SpiBusDevice.TYPE_ID,
  typeName: SpiBusDevice.TYPE_NAME,
  labelKey: 'device.type.spiBus',
  icon: 'bus',
  detailComponent: SpiBusDeviceDetail,
  formComponent: SpiBusDeviceForm,
  editorComponent: SpiBusDeviceEditor,
  widgetComponent: SpiBusDeviceWidget,
}
