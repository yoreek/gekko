import St7735DeviceDetail from './St7735DeviceDetail.vue'
import St7735DeviceForm from './St7735DeviceForm.vue'
import St7735DeviceEditor from './St7735DeviceEditor.vue'
import St7735DeviceWidget from './St7735DeviceWidget.vue'
import St7735DesignerDialog from './St7735DesignerDialog.vue'
import { St7735Device } from '@/models/devices/st7735/device'
import type { DeviceUi } from '@/components/devices/registry/device-ui-types'

export const st7735Ui: DeviceUi = {
  typeId: St7735Device.TYPE_ID,
  typeName: St7735Device.TYPE_NAME,
  labelKey: 'device.type.st7735',
  icon: 'device',
  detailComponent: St7735DeviceDetail,
  formComponent: St7735DeviceForm,
  editorComponent: St7735DeviceEditor,
  widgetComponent: St7735DeviceWidget,
  designerComponent: St7735DesignerDialog,
  designDisplayLabelKey: 'device.dialog.st7735Display.designDisplay',
}
