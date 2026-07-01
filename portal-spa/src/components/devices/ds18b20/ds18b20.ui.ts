import Ds18b20TemperatureSensorDeviceDetail from './Ds18b20TemperatureSensorDeviceDetail.vue'
import Ds18b20TemperatureSensorDeviceForm from './Ds18b20TemperatureSensorDeviceForm.vue'
import Ds18b20TemperatureSensorDeviceEditor from './Ds18b20TemperatureSensorDeviceEditor.vue'
import Ds18b20TemperatureSensorDeviceWidget from './Ds18b20TemperatureSensorDeviceWidget.vue'
import { Ds18b20Device } from '@/models/devices/ds18b20'
import type { DeviceUi } from '@/components/devices/registry/device-ui-types'

export const ds18b20Ui: DeviceUi = {
  typeId: Ds18b20Device.TYPE_ID,
  typeName: Ds18b20Device.TYPE_NAME,
  labelKey: 'device.type.ds18b20TemperatureSensor',
  icon: 'temperature',
  detailComponent: Ds18b20TemperatureSensorDeviceDetail,
  formComponent: Ds18b20TemperatureSensorDeviceForm,
  editorComponent: Ds18b20TemperatureSensorDeviceEditor,
  widgetComponent: Ds18b20TemperatureSensorDeviceWidget,
}
