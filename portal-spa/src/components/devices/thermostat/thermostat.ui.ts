import ThermostatDeviceEditor from './ThermostatDeviceEditor.vue'
import ThermostatDeviceWidget from './ThermostatDeviceWidget.vue'
import { ThermostatDevice } from '@/models/devices/thermostat'
import type { DeviceUi } from '@/components/devices/registry/device-ui-types'

export const thermostatUi: DeviceUi = {
  typeId: ThermostatDevice.TYPE_ID,
  typeName: ThermostatDevice.TYPE_NAME,
  labelKey: 'device.type.thermostat',
  icon: 'temperature',
  editorComponent: ThermostatDeviceEditor,
  widgetComponent: ThermostatDeviceWidget,
}
