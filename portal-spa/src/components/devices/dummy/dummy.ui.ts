import DummyDeviceDetail from './DummyDeviceDetail.vue'
import DummyDeviceEditor from './DummyDeviceEditor.vue'
import DummyDeviceWidget from './DummyDeviceWidget.vue'
import { DummyDevice } from '@/models/devices/dummy'
import type { DeviceUi } from '@/components/devices/registry/device-ui-types'

export const dummyUi: DeviceUi = {
  typeId: DummyDevice.TYPE_ID,
  typeName: DummyDevice.TYPE_NAME,
  labelKey: 'device.type.dummy',
  icon: 'device',
  detailComponent: DummyDeviceDetail,
  editorComponent: DummyDeviceEditor,
  widgetComponent: DummyDeviceWidget,
}
