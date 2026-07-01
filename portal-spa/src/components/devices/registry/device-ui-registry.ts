import type { DeviceUi } from './device-ui-types'
import { deviceTypeIdFromName } from '@/models/device-type-ids'
import { dummyUi } from '@/components/devices/dummy/dummy.ui'
import { gpioSwitchUi } from '@/components/devices/gpio-switch/gpio-switch.ui'
import { oneWireBusUi } from '@/components/devices/onewire-bus/onewire-bus.ui'
import { i2cBusUi } from '@/components/devices/i2c-bus/i2c-bus.ui'
import { spiBusUi } from '@/components/devices/spi-bus/spi-bus.ui'
import { ssd1306Ui } from '@/components/devices/display/ssd1306/ssd1306.ui'
import { st7735Ui } from '@/components/devices/display/st7735/st7735.ui'
import { ds18b20Ui } from '@/components/devices/ds18b20/ds18b20.ui'
import { thermostatUi } from '@/components/devices/thermostat/thermostat.ui'

const unknownUi: DeviceUi = {
  typeId: 0,
  typeName: '',
  labelKey: 'device.type.unknown',
  icon: 'device',
  editorComponent: dummyUi.editorComponent,
  widgetComponent: dummyUi.widgetComponent,
}

const deviceUiByTypeId: Record<number, DeviceUi> = {
  [dummyUi.typeId]: dummyUi,
  [gpioSwitchUi.typeId]: gpioSwitchUi,
  [oneWireBusUi.typeId]: oneWireBusUi,
  [i2cBusUi.typeId]: i2cBusUi,
  [spiBusUi.typeId]: spiBusUi,
  [ssd1306Ui.typeId]: ssd1306Ui,
  [st7735Ui.typeId]: st7735Ui,
  [ds18b20Ui.typeId]: ds18b20Ui,
  [thermostatUi.typeId]: thermostatUi,
}

export const allDeviceUis: DeviceUi[] = Object.values(deviceUiByTypeId)

export function resolveDeviceUiByTypeId(typeId: number): DeviceUi {
  return deviceUiByTypeId[typeId] ?? unknownUi
}

export function resolveDeviceUi(typeIdOrName: number | string | undefined | null): DeviceUi {
  return typeof typeIdOrName === 'number'
    ? resolveDeviceUiByTypeId(typeIdOrName)
    : resolveDeviceUiByTypeId(deviceTypeIdFromName(typeIdOrName))
}
