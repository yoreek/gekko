import type { BaseDeviceConfig, DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice } from '@/models/devices/base-device'
import { isSupportedRasterFormat, type DisplayBaseConfig, type DisplayCapabilities } from '@/models/devices/display/base'
import {
  defaultSt7735Layout,
  encodeSt7735Layout,
  normalizeSt7735Layout,
  type St7735LayoutDraft,
} from './layout.ts'

export interface St7735ConfigDraft extends BaseDeviceConfig, DisplayBaseConfig {
  layout: St7735LayoutDraft
}

export interface St7735CreateDraft extends DeviceCreateDraftBase, St7735ConfigDraft {}

export const displayCapabilities: DisplayCapabilities = {
  supportedRasterFormats: ['rgb565'],
  defaultRasterFormat: 'rgb565',
  supportsBitmapImport: true,
  supportsAspectRatioLock: true,
  maxBitmapBytes: 128 * 160 * 2,
}

export const ST7735_BITMAP_DEFAULT_WIDTH = 16
export const ST7735_BITMAP_DEFAULT_HEIGHT = 16

export function defaultConfig(): St7735ConfigDraft {
  return {
    enabled: true,
    name: 'New Display',
    deps: [],
    width: 128,
    height: 160,
    layout: defaultSt7735Layout(),
  }
}

export function normalizeConfig(value: unknown): St7735ConfigDraft {
  const defaults = defaultConfig()
  if (typeof value !== 'object' || value === null || Array.isArray(value)) return defaults
  const raw = value as Record<string, unknown>
  return {
    ...defaults,
    name: typeof raw.name === 'string' ? raw.name : defaults.name,
    enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
    width: typeof raw.width === 'number' ? raw.width : defaults.width,
    height: typeof raw.height === 'number' ? raw.height : defaults.height,
    layout: normalizeSt7735Layout(raw.layout),
  }
}

export function encodeConfig(config: St7735ConfigDraft): Record<string, unknown> {
  return {
    ...config,
    layout: encodeSt7735Layout(config.layout),
  }
}

export class Device extends BaseDevice<St7735ConfigDraft, St7735CreateDraft, Record<string, never>> {
  readonly typeName = 'st7735'
  readonly typeId = 8
  readonly displayCapabilities = displayCapabilities

  createDefaultConfig(): St7735ConfigDraft { return defaultConfig() }
  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): St7735CreateDraft { return { ...this.createDefaultConfig(), ...common, typeName: common.typeName ?? this.typeName } }
  createEditDraft(current: DeviceRecord): St7735CreateDraft { return { ...this.normalizeConfig(current.config), typeName: this.typeName } }
  normalizeConfig(value: unknown): St7735ConfigDraft { return normalizeConfig(value) }
  normalizeOutput(): Record<string, never> { return {} }
  override encodeConfig(config: St7735ConfigDraft): Record<string, unknown> { return encodeConfig(config) }
  buildEditCommands(): DeviceCommandRequest[] { return [] }
  protected extractCreateConfig(draft: St7735CreateDraft): St7735ConfigDraft {
    const { typeName: _typeName, ...config } = draft
    return config
  }
}

export function supportsBitmapFormat(format: string): format is 'rgb565' {
  return isSupportedRasterFormat(displayCapabilities, format as never)
}
