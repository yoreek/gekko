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
import type { OutputState } from '@/models/devices/switch'

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

export interface EncodedGpioSwitchConfigDraft extends GpioSwitchConfigDraft {
  enabled: boolean
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

export function encodeGpioSwitchConfigBlob(config: EncodedGpioSwitchConfigDraft): string {
  const bytes = new Uint8Array(10)
  const magicKey = 0x47535731
  bytes[0] = magicKey & 0xff
  bytes[1] = (magicKey >> 8) & 0xff
  bytes[2] = (magicKey >> 16) & 0xff
  bytes[3] = (magicKey >> 24) & 0xff
  bytes[4] = config.enabled ? 1 : 0
  bytes[5] = config.restore_previous_state ? 1 : 0
  bytes[6] = config.startup_state === 'on' ? 1 : config.startup_state === 'disabled' ? 2 : 0
  bytes[7] = config.safe_state === 'on' ? 1 : config.safe_state === 'disabled' ? 2 : 0
  bytes[8] = config.inverted ? 1 : 0
  bytes[9] = config.gpio_pin & 0xff
  return Array.from(bytes, byte => String.fromCharCode(byte)).join('')
}

export function decodeGpioSwitchConfigBlob(blob: string): EncodedGpioSwitchConfigDraft | null {
  if (blob.length !== 10) {
    return null
  }
  const bytes = Uint8Array.from(blob, char => char.charCodeAt(0))
  const magicKey = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24)
  if (magicKey !== 0x47535731) {
    return null
  }
  const startupState = byteToOutputState(bytes[6])
  const safeState = byteToOutputState(bytes[7])
  if (startupState === null || safeState === null) {
    return null
  }
  return {
    enabled: bytes[4] !== 0,
    restore_previous_state: bytes[5] !== 0,
    startup_state: startupState,
    safe_state: safeState,
    inverted: bytes[8] !== 0,
    gpio_pin: bytes[9],
  }
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
      payload: name,
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
        payload: encodeGpioSwitchConfigBlob({
          ...payload.gpioSwitchConfig,
          enabled: payload.common.enabled,
        }),
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
        payload: encodeOneWireBusConfigBlob({
          ...payload.oneWireBusConfig,
          enabled: payload.common.enabled,
        }),
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
        has_parent: true,
        parent_device_id: next.parent_device_id,
      })
    }
  }

  return commands
}

function byteToOutputState(value: number): OutputState | null {
  if (value === 0) {
    return 'off'
  }
  if (value === 1) {
    return 'on'
  }
  if (value === 2) {
    return 'disabled'
  }
  return null
}
function encodeOneWireBusConfigBlob(config: OneWireBusConfigDraft): string {
  const bytes = new Uint8Array(7)
  const magicKey = 0x4f573131
  bytes[0] = magicKey & 0xff
  bytes[1] = (magicKey >> 8) & 0xff
  bytes[2] = (magicKey >> 16) & 0xff
  bytes[3] = (magicKey >> 24) & 0xff
  bytes[4] = config.enabled ? 1 : 0
  bytes[5] = config.gpio_pin & 0xff
  bytes[6] = config.internal_pullup ? 1 : 0
  return Array.from(bytes, byte => String.fromCharCode(byte)).join('')
}
