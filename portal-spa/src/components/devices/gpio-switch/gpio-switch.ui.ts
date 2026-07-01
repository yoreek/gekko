import GpioSwitchDeviceDetail from './GpioSwitchDeviceDetail.vue'
import GpioSwitchDeviceForm from './GpioSwitchDeviceForm.vue'
import GpioSwitchDeviceEditor from './GpioSwitchDeviceEditor.vue'
import GpioSwitchDeviceWidget from './GpioSwitchDeviceWidget.vue'
import { GpioSwitchDevice } from '@/models/devices/gpio-switch'
import type { DeviceUi } from '@/components/devices/registry/device-ui-types'

export const gpioSwitchUi: DeviceUi = {
  typeId: GpioSwitchDevice.TYPE_ID,
  typeName: GpioSwitchDevice.TYPE_NAME,
  labelKey: 'device.type.gpioSwitch',
  icon: 'power',
  detailComponent: GpioSwitchDeviceDetail,
  formComponent: GpioSwitchDeviceForm,
  editorComponent: GpioSwitchDeviceEditor,
  widgetComponent: GpioSwitchDeviceWidget,
}
