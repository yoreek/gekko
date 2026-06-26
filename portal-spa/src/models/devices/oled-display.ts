import type { BaseDeviceConfig, DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice } from '@/models/devices/base-device'
import {
  defaultOledDisplayLayout,
  encodeOledDisplayLayout,
  oledDisplayLayoutChanged,
  normalizeOledDisplayLayout,
  type OledDisplayLayoutDraft,
} from '@/models/devices/oled-display-layout'

export namespace OledDisplay {
  export interface ConfigDraft extends BaseDeviceConfig {
    i2cBusDeviceId: number
    i2cAddress: number
    layoutWidth: number
    layoutHeight: number
    layout: OledDisplayLayoutDraft
  }

  export interface CreateDraft extends DeviceCreateDraftBase, ConfigDraft {}

  export function defaultConfig(): ConfigDraft {
    return {
      enabled: true,
      name: 'New Device',
      deps: [],
      i2cBusDeviceId: 0,
      i2cAddress: 60,
      layoutWidth: 128,
      layoutHeight: 64,
      layout: defaultOledDisplayLayout(),
    }
  }

  export function normalizeConfig(value: unknown): ConfigDraft {
    const defaults = defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) return defaults
    const raw = value as Record<string, unknown>
    const layoutRaw = raw.layout
    const layoutObject = typeof layoutRaw === 'object' && layoutRaw !== null && !Array.isArray(layoutRaw) ? (layoutRaw as Record<string, unknown>) : null
    return {
      name: typeof raw.name === 'string' ? raw.name : defaults.name,
      enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
      deps: Array.isArray(raw.deps) ? (raw.deps as ConfigDraft['deps']) : defaults.deps,
      i2cBusDeviceId: typeof raw.i2cBusDeviceId === 'number' ? raw.i2cBusDeviceId : defaults.i2cBusDeviceId,
      i2cAddress: typeof raw.i2cAddress === 'number' ? raw.i2cAddress : defaults.i2cAddress,
      layoutWidth: typeof raw.layoutWidth === 'number' ? raw.layoutWidth : defaults.layoutWidth,
      layoutHeight: typeof raw.layoutHeight === 'number' ? raw.layoutHeight : defaults.layoutHeight,
      layout: normalizeOledDisplayLayout(layoutObject ?? defaults.layout),
    }
  }

  export function encodeConfig(config: ConfigDraft): Record<string, unknown> {
    return {
      ...config,
      layout: encodeOledDisplayLayout(config.layout),
    }
  }

  export function formatI2cAddress(value: number): string {
    return value.toString(16).toUpperCase().padStart(2, '0')
  }

  export function parseI2cAddress(value: string | number): number {
    const text = String(value).trim().replace(/^0x/i, '')
    if (!/^[0-9a-fA-F]{1,2}$/.test(text)) {
      return Number.NaN
    }
    return Number.parseInt(text, 16)
  }

  export class Device extends BaseDevice<ConfigDraft, CreateDraft, Record<string, never>> {
    readonly typeName = 'oled_display'
    readonly typeId = 7

    createDefaultConfig(): ConfigDraft { return defaultConfig() }
    createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): CreateDraft { return { ...this.createDefaultConfig(), ...common, typeName: common.typeName ?? this.typeName } }
    createEditDraft(current: DeviceRecord): CreateDraft { return { ...this.normalizeConfig(current.config), typeName: this.typeName } }
    normalizeConfig(value: unknown): ConfigDraft { return normalizeConfig(value) }
    normalizeOutput(): Record<string, never> { return {} }
    override encodeConfig(config: ConfigDraft): Record<string, unknown> { return encodeConfig(config) }
    buildEditCommands(current: DeviceRecord, draft: CreateDraft): DeviceCommandRequest[] {
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
        nextConfig.layoutWidth !== currentConfig.layoutWidth ||
        nextConfig.layoutHeight !== currentConfig.layoutHeight ||
        oledDisplayLayoutChanged(nextConfig.layout, currentConfig.layout)
      ) {
        commands.push({ command: 'updateConfig', config: this.encodeConfig(nextConfig) })
      }
      return commands
    }
    protected extractCreateConfig(draft: CreateDraft): ConfigDraft {
      const { typeName: _typeName, ...config } = draft
      return config
    }
  }
}
