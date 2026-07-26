import type { BaseDeviceConfig, DeviceDependencyLink, DeviceRecord } from '../../../api/contracts.ts'
import type { DeviceCreateDraftBase } from '../base.ts'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from '../base-device.ts'
import { ssd1306Display } from '../display/display.ts'
import type { DisplayBaseConfig, DisplayCapabilities } from '../display/base.ts'
import { normalizeDisplayRotation } from '../display/orientation.ts'
import { SSD1306_DEFAULT_PANEL, SSD1306_CUSTOM_PANEL, isKnownPanel, resolvePanelGeometry, matchPanelByGeometry } from '../display/panels.ts'
import {
  defaultSsd1306Layout,
  encodeSsd1306Layout,
  normalizeSsd1306Layout,
  type Ssd1306LayoutDraft,
} from './layout.ts'

export interface Ssd1306ConfigDraft extends BaseDeviceConfig, DisplayBaseConfig {
  dependencyDeviceId: number
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
      ...defaultBaseDeviceConfig(),
      dependencyDeviceId: 0,
      i2cAddress: 60,
      rotation: 0,
      panel: SSD1306_DEFAULT_PANEL,
      width: 128,
      height: 64,
      layout: defaultSsd1306Layout(),
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Ssd1306ConfigDraft {
    const defaults = Ssd1306Device.defaultConfig()
    const dependencyDeviceId = deps?.find(dep => dep.role === 'i2c_bus')?.deviceId ?? 0
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return { ...defaults, deps: deps ?? defaults.deps, dependencyDeviceId }
    }
    const raw = value as Record<string, unknown>
    const layoutRaw = raw.layout
    const layoutObject = typeof layoutRaw === 'object' && layoutRaw !== null && !Array.isArray(layoutRaw) ? (layoutRaw as Record<string, unknown>) : null
    const rawWidth = typeof raw.width === 'number' ? raw.width : defaults.width
    const rawHeight = typeof raw.height === 'number' ? raw.height : defaults.height
    // Prefer an explicit valid panel; otherwise fall back to matching width/height against the
    // preset table (mirrors the firmware's V5->V6 migration heuristic), else Custom.
    const panel = isKnownPanel('ssd1306', raw.panel)
      ? (raw.panel as string)
      : (matchPanelByGeometry('ssd1306', rawWidth, rawHeight) ?? SSD1306_CUSTOM_PANEL)
    const geometry = resolvePanelGeometry('ssd1306', panel)
    return {
      ...normalizeBaseDeviceConfig(raw, defaults),
      dependencyDeviceId,
      i2cAddress: typeof raw.i2cAddress === 'number' ? raw.i2cAddress : defaults.i2cAddress,
      rotation: normalizeDisplayRotation(raw.rotation, defaults.rotation),
      panel,
      width: geometry?.width ?? rawWidth,
      height: geometry?.height ?? rawHeight,
      layout: normalizeSsd1306Layout(layoutObject ?? defaults.layout),
    }
  }

  static encodeConfig(config: Ssd1306ConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      i2cAddress: config.i2cAddress,
      rotation: config.rotation,
      panel: config.panel,
      width: config.width,
      height: config.height,
      layout: encodeSsd1306Layout(config.layout),
    }
  }

  static supportsBitmapFormat(format: string): format is 'mono1' {
    return ssd1306Display.supportsBitmapFormat(format as never)
  }

  createDefaultConfig(): Ssd1306ConfigDraft { return Ssd1306Device.defaultConfig() }
  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Ssd1306CreateDraft { return { ...this.createDefaultConfig(), ...common, typeName: common.typeName ?? this.typeName } }
  createEditDraft(current: DeviceRecord): Ssd1306CreateDraft {
    return { ...this.normalizeConfig(current.config, current.config.deps), typeName: this.typeName }
  }
  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Ssd1306ConfigDraft { return Ssd1306Device.normalizeConfig(value, deps) }
  normalizeOutput(): Record<string, never> { return {} }
  protected override encodeConfig(config: Ssd1306ConfigDraft): Record<string, unknown> { return Ssd1306Device.encodeConfig(config) }

  protected override createCreateDeps(config: Ssd1306ConfigDraft): DeviceDependencyLink[] {
    return [{ role: 'i2c_bus', deviceId: config.dependencyDeviceId }]
  }
}
