import type { DeviceCommandRequest } from '@/api'
import type { DashboardDevice } from '@/models/device'
import {
  DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
  DUMMY_DEVICE_TYPE_ID,
  GPIO_SWITCH_DEVICE_TYPE_ID,
  ONEWIRE_BUS_DEVICE_TYPE_ID,
} from '@/models/device-types'
import {
  createDefaultDs18b20TemperatureSensorConfig,
  ds18b20ConfigChanged,
  encodeDs18b20Config,
  normalizeDs18b20TemperatureSensorConfig,
  type Ds18b20TemperatureSensorConfigDraft,
} from '@/models/devices/ds18b20'
import { createDefaultGpioSwitchConfig, normalizeGpioSwitchConfig, type GpioSwitchConfigDraft } from '@/models/devices/gpio-switch'
import { createDefaultOneWireBusConfig, normalizeOneWireBusConfig, type OneWireBusConfigDraft } from '@/models/devices/onewire-bus'

export interface DeviceCommonDraft {
  name: string
  typeId: number
  enabled: boolean
}

export interface DeviceEditDraft {
  common: DeviceCommonDraft
  gpioSwitchConfig: GpioSwitchConfigDraft
  oneWireBusConfig: OneWireBusConfigDraft
  ds18b20Config: Ds18b20TemperatureSensorConfigDraft
}

export interface DeviceEditSubmitPayload {
  common: DeviceCommonDraft
  gpioSwitchConfig?: GpioSwitchConfigDraft
  oneWireBusConfig?: OneWireBusConfigDraft
  ds18b20Config?: Ds18b20TemperatureSensorConfigDraft
}

export function createDefaultDeviceCommonDraft(): DeviceCommonDraft {
  return {
    name: 'New Device',
    typeId: DUMMY_DEVICE_TYPE_ID,
    enabled: true,
  }
}

export function createDeviceCommonDraft(device: DashboardDevice | null): DeviceCommonDraft {
  if (device === null) {
    return createDefaultDeviceCommonDraft()
  }
  return {
    name: device.name,
    typeId: device.typeId,
    enabled: device.enabled,
  }
}

export function createDefaultDeviceConfigDraft(typeId: number): Record<string, unknown> {
  if (typeId === GPIO_SWITCH_DEVICE_TYPE_ID) {
    return { ...createDefaultGpioSwitchConfig() }
  }
  if (typeId === ONEWIRE_BUS_DEVICE_TYPE_ID) {
    return { ...createDefaultOneWireBusConfig() }
  }
  if (typeId === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID) {
    return { ...createDefaultDs18b20TemperatureSensorConfig() }
  }
  return {}
}

export function isGpioSwitchType(typeId: number): boolean {
  return typeId === GPIO_SWITCH_DEVICE_TYPE_ID
}

export function isOneWireBusType(typeId: number): boolean {
  return typeId === ONEWIRE_BUS_DEVICE_TYPE_ID
}

export function isDs18b20Type(typeId: number): boolean {
  return typeId === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID
}

export function createDeviceEditDraft(device: DashboardDevice | null): DeviceEditDraft {
  const gpioSwitchConfig = normalizeGpioSwitchConfig(device?.detail.config)
  const oneWireBusConfig = normalizeOneWireBusConfig(device?.detail.config)
  const ds18b20Config = normalizeDs18b20TemperatureSensorConfig(device?.detail.config, device?.parentDeviceId)
  if (device === null) {
    return {
      common: createDefaultDeviceCommonDraft(),
      gpioSwitchConfig,
      oneWireBusConfig,
      ds18b20Config,
    }
  }
  const common = createDeviceCommonDraft(device)
  return {
    common,
    gpioSwitchConfig: isGpioSwitchType(device.typeId) ? gpioSwitchConfig : createDefaultGpioSwitchConfig(),
    oneWireBusConfig: isOneWireBusType(device.typeId) ? oneWireBusConfig : createDefaultOneWireBusConfig(),
    ds18b20Config: isDs18b20Type(device.typeId) ? ds18b20Config : createDefaultDs18b20TemperatureSensorConfig(),
  }
}

export function normalizeGpioSwitchDraft(value: unknown): GpioSwitchConfigDraft {
  return normalizeGpioSwitchConfig(value)
}

export function gpioSwitchConfigChanged(current: GpioSwitchConfigDraft, original: GpioSwitchConfigDraft): boolean {
  return (
    current.restore_previous_state !== original.restore_previous_state ||
    current.startup_state !== original.startup_state ||
    current.safe_state !== original.safe_state ||
    current.inverted !== original.inverted ||
    current.gpio_pin !== original.gpio_pin
  )
}

export function buildDeviceEditCommands(device: DashboardDevice, payload: DeviceEditSubmitPayload): DeviceCommandRequest[] {
  const commands: DeviceCommandRequest[] = []
  const name = payload.common.name.trim()
  if (name !== device.name) {
    commands.push({
      command: 'rename',
      name,
    })
  }

  if (payload.common.enabled !== device.enabled) {
    commands.push({
      command: payload.common.enabled ? 'enable' : 'disable',
    })
  }

  if (isGpioSwitchType(device.typeId) && payload.gpioSwitchConfig !== undefined) {
    const current = normalizeGpioSwitchConfig(device.detail.config)
    if (payload.common.enabled !== device.enabled || gpioSwitchConfigChanged(payload.gpioSwitchConfig, current)) {
      commands.push({
        command: 'update_config',
        config: {
          ...payload.gpioSwitchConfig,
          enabled: payload.common.enabled,
        },
      })
    }
  }

  if (isOneWireBusType(device.typeId) && payload.oneWireBusConfig !== undefined) {
    const current = normalizeOneWireBusConfig(device.detail.config)
    if (
      payload.common.enabled !== device.enabled ||
      current.gpio_pin !== payload.oneWireBusConfig.gpio_pin ||
      current.internal_pullup !== payload.oneWireBusConfig.internal_pullup ||
      current.enabled !== payload.oneWireBusConfig.enabled
    ) {
      commands.push({
        command: 'update_config',
        config: {
          ...payload.oneWireBusConfig,
          enabled: payload.common.enabled,
        },
      })
    }
  }

  if (isDs18b20Type(device.typeId) && payload.ds18b20Config !== undefined) {
    const current = normalizeDs18b20TemperatureSensorConfig(device.detail.config, device.parentDeviceId)
    const next = {
      ...payload.ds18b20Config,
      enabled: payload.common.enabled,
    }
    if (ds18b20ConfigChanged(next, current)) {
      commands.push({
        command: 'update_config',
        config: encodeDs18b20Config(next),
        deps: [
          {
            role: 'onewire_bus',
            device_id: next.parent_device_id,
          },
        ],
      })
    }
  }

  return commands
}
