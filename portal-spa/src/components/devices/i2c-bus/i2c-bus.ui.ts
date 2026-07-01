import I2cBusDeviceDetail from './I2cBusDeviceDetail.vue'
import I2cBusDeviceForm from './I2cBusDeviceForm.vue'
import I2cBusDeviceWidget from './I2cBusDeviceWidget.vue'
import { I2cBusDevice } from '@/models/devices/i2c-bus'
import type { DeviceUi } from '@/components/devices/registry/device-ui-types'

export const i2cBusUi: DeviceUi = {
  typeId: I2cBusDevice.TYPE_ID,
  typeName: I2cBusDevice.TYPE_NAME,
  labelKey: 'device.type.i2cBus',
  icon: 'bus',
  detailComponent: I2cBusDeviceDetail,
  formComponent: I2cBusDeviceForm,
  widgetComponent: I2cBusDeviceWidget,
}
