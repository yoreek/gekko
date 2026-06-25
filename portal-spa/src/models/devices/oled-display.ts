import type { BaseDeviceConfig, DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice } from '@/models/devices/base-device'

export namespace OledDisplay {
  export interface LayoutPage {
    id: string
    name: string
    order: number
    widgets: unknown[]
  }

  export interface LayoutDraft {
    schemaVersion: number
    activePageId: string
    pages: LayoutPage[]
  }

  export interface ConfigDraft extends BaseDeviceConfig {
    i2cBusDeviceId: number
    i2cAddress: number
    layoutWidth: number
    layoutHeight: number
    layout: LayoutDraft
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
      layout: { schemaVersion: 1, activePageId: 'main', pages: [] },
    }
  }

  export function normalizeConfig(value: unknown): ConfigDraft {
    const defaults = defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) return defaults
    const raw = value as Record<string, unknown>
    return {
      name: typeof raw.name === 'string' ? raw.name : defaults.name,
      enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
      deps: Array.isArray(raw.deps) ? (raw.deps as ConfigDraft['deps']) : defaults.deps,
      i2cBusDeviceId: typeof raw.i2cBusDeviceId === 'number' ? raw.i2cBusDeviceId : defaults.i2cBusDeviceId,
      i2cAddress: typeof raw.i2cAddress === 'number' ? raw.i2cAddress : defaults.i2cAddress,
      layoutWidth: typeof raw.layoutWidth === 'number' ? raw.layoutWidth : defaults.layoutWidth,
      layoutHeight: typeof raw.layoutHeight === 'number' ? raw.layoutHeight : defaults.layoutHeight,
      layout:
        typeof raw.layout === 'object' && raw.layout !== null && !Array.isArray(raw.layout)
          ? {
              schemaVersion:
                typeof (raw.layout as Record<string, unknown>).schemaVersion === 'number'
                  ? ((raw.layout as Record<string, unknown>).schemaVersion as number)
                  : defaults.layout.schemaVersion,
              activePageId:
                typeof (raw.layout as Record<string, unknown>).activePageId === 'string'
                  ? ((raw.layout as Record<string, unknown>).activePageId as string)
                  : defaults.layout.activePageId,
              pages: Array.isArray((raw.layout as Record<string, unknown>).pages)
                ? ((raw.layout as Record<string, unknown>).pages as LayoutPage[])
                : defaults.layout.pages,
            }
          : defaults.layout,
    }
  }

  export function encodeConfig(config: ConfigDraft): Record<string, unknown> {
    return { ...config }
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
      if (nextConfig.i2cBusDeviceId !== currentConfig.i2cBusDeviceId || nextConfig.i2cAddress !== currentConfig.i2cAddress || nextConfig.layoutWidth !== currentConfig.layoutWidth || nextConfig.layoutHeight !== currentConfig.layoutHeight) {
        commands.push({ command: 'updateConfig', config: this.encodeConfig(nextConfig) })
      }
      return commands
    }
    protected extractCreateConfig(draft: CreateDraft): ConfigDraft { return { ...draft } }
  }
}
