import Ssd1306DeviceDetail from './Ssd1306DeviceDetail.vue'
import Ssd1306DeviceForm from './Ssd1306DeviceForm.vue'
import Ssd1306DesignerDialog from './Ssd1306DesignerDialog.vue'
import DisplayDeviceWidget from '@/components/devices/display/DisplayDeviceWidget.vue'
import { Ssd1306Device } from '@/models/devices/ssd1306/device'
import type { DeviceUi } from '@/components/devices/registry/device-ui-types'

export const ssd1306Ui: DeviceUi = {
  typeId: Ssd1306Device.TYPE_ID,
  typeName: Ssd1306Device.TYPE_NAME,
  labelKey: 'device.type.ssd1306Display',
  icon: 'device',
  detailComponent: Ssd1306DeviceDetail,
  formComponent: Ssd1306DeviceForm,
  widgetComponent: DisplayDeviceWidget,
  designerComponent: Ssd1306DesignerDialog,
  designDisplayLabelKey: 'device.dialog.ssd1306Display.designDisplay',
  layoutResizeWarningLabelKey: 'device.dialog.ssd1306Display.layoutResizeWarning',
}
