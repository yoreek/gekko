import type { BaseDeviceConfig, DeviceCommandRequest, DeviceRecord } from '../../../api/contracts.ts'
import type { DeviceCreateDraftBase } from '../base.ts'
import { BaseDevice } from '../base-device.ts'
import { ssd1306Display } from '../display/display.ts'
import type { DisplayBaseConfig, DisplayCapabilities } from '../display/base.ts'
import { normalizeDisplayRotation } from '../display/orientation.ts'
import {
  defaultSsd1306Layout,
  encodeSsd1306Layout,
  normalizeSsd1306Layout,
  ssd1306LayoutChanged,
  type Ssd1306LayoutDraft,
} from './layout.ts'

export interface Ssd1306ConfigDraft extends BaseDeviceConfig, DisplayBaseConfig {
  i2cBusDeviceId: number
  i2cAddress: number
  rotation: number
  layout: Ssd1306LayoutDraft
}

export interface Ssd1306CreateDraft extends DeviceCreateDraftBase, Ssd1306ConfigDraft {}

export class Ssd1306Device extends BaseDevice<Ssd1306ConfigDraft, Ssd1306CreateDraft, Record<string, never>> {
  static readonly TYPE_ID = 7 as const
  static readonly TYPE_NAME = 'ssd1306' as const
  static readonly displayCapabilities: DisplayCapabilities = ssd1306Display.displayCapabilities

  readonly typeName = Ssd1306Device.TYPE_NAME
  readonly typeId = Ssd1306Device.TYPE_ID
  readonly displayCapabilities = Ssd1306Device.displayCapabilities

  static defaultConfig(): Ssd1306ConfigDraft {
    return {
      enabled: true,
      name: 'New Device',
      deps: [],
      i2cBusDeviceId: 0,
      i2cAddress: 60,
      rotation: 0,
      width: 128,
      height: 64,
      layout: defaultSsd1306Layout(),
    }
  }

  static normalizeConfig(value: unknown): Ssd1306ConfigDraft {
    const defaults = Ssd1306Device.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) return defaults
    const raw = value as Record<string, unknown>
    const layoutRaw = raw.layout
    const layoutObject = typeof layoutRaw === 'object' && layoutRaw !== null && !Array.isArray(layoutRaw) ? (layoutRaw as Record<string, unknown>) : null
    return {
      name: typeof raw.name === 'string' ? raw.name : defaults.name,
      enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
      deps: Array.isArray(raw.deps) ? (raw.deps as Ssd1306ConfigDraft['deps']) : defaults.deps,
      i2cBusDeviceId: typeof raw.i2cBusDeviceId === 'number' ? raw.i2cBusDeviceId : defaults.i2cBusDeviceId,
      i2cAddress: typeof raw.i2cAddress === 'number' ? raw.i2cAddress : defaults.i2cAddress,
      rotation: normalizeDisplayRotation(raw.rotation, defaults.rotation) % 2,
      width: typeof raw.width === 'number' ? raw.width : defaults.width,
      height: typeof raw.height === 'number' ? raw.height : defaults.height,
      layout: normalizeSsd1306Layout(layoutObject ?? defaults.layout),
    }
  }

  static encodeConfig(config: Ssd1306ConfigDraft): Record<string, unknown> {
    return {
      ...config,
      layout: encodeSsd1306Layout(config.layout),
    }
  }

  static supportsBitmapFormat(format: string): format is 'mono1' {
    return ssd1306Display.supportsBitmapFormat(format as never)
  }

  createDefaultConfig(): Ssd1306ConfigDraft { return Ssd1306Device.defaultConfig() }
  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Ssd1306CreateDraft { return { ...this.createDefaultConfig(), ...common, typeName: common.typeName ?? this.typeName } }
  createEditDraft(current: DeviceRecord): Ssd1306CreateDraft { return { ...this.normalizeConfig(current.config), typeName: this.typeName } }
  normalizeConfig(value: unknown): Ssd1306ConfigDraft { return Ssd1306Device.normalizeConfig(value) }
  normalizeOutput(): Record<string, never> { return {} }
  override encodeConfig(config: Ssd1306ConfigDraft): Record<string, unknown> { return Ssd1306Device.encodeConfig(config) }
  buildEditCommands(current: DeviceRecord, draft: Ssd1306CreateDraft): DeviceCommandRequest[] {
    const currentConfig = this.normalizeConfig(current.config)
    const commands: DeviceCommandRequest[] = []
    if (draft.name.trim() !== currentConfig.name) commands.push({ command: 'rename', name: draft.name.trim() })
    if (draft.enabled !== currentConfig.enabled) commands.push({ command: draft.enabled ? 'enable' : 'disable' })
    const nextConfig = this.normalizeConfig(draft)
    if (nextConfig.i2cBusDeviceId !== currentConfig.i2cBusDeviceId) {
      commands.push({
        command: 'setDeps',
        deps: [
          {
            role: 'i2c_bus',
            deviceId: nextConfig.i2cBusDeviceId,
          },
        ],
      })
    }
    if (
      nextConfig.i2cBusDeviceId !== currentConfig.i2cBusDeviceId ||
      nextConfig.i2cAddress !== currentConfig.i2cAddress ||
      nextConfig.rotation !== currentConfig.rotation ||
      nextConfig.width !== currentConfig.width ||
      nextConfig.height !== currentConfig.height ||
      ssd1306LayoutChanged(nextConfig.layout, currentConfig.layout)
    ) {
      commands.push({ command: 'updateConfig', config: this.encodeConfig(nextConfig) })
    }
    return commands
  }
  protected extractCreateConfig(draft: Ssd1306CreateDraft): Ssd1306ConfigDraft {
    const { typeName: _typeName, ...config } = draft
    return config
  }
}
