import SpiBusDeviceEditor from './SpiBusDeviceEditor.vue'
import SpiBusDeviceWidget from './SpiBusDeviceWidget.vue'
import { SpiBusDevice } from '@/models/devices/spi-bus'
import type { DeviceUi } from '@/components/devices/registry/device-ui-types'

export const spiBusUi: DeviceUi = {
  typeId: SpiBusDevice.TYPE_ID,
  typeName: SpiBusDevice.TYPE_NAME,
  labelKey: 'device.type.spiBus',
  icon: 'bus',
  editorComponent: SpiBusDeviceEditor,
  widgetComponent: SpiBusDeviceWidget,
}
