import type { BaseDeviceConfig, DeviceDependencyLink, DeviceRecord } from '../../../api/contracts.ts'
import type { DeviceCreateDraftBase } from '../base.ts'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from '../base-device.ts'
import { st7735Display } from '../display/display.ts'
import type { DisplayBaseConfig, DisplayCapabilities } from '../display/base.ts'
import { normalizeDisplayRotation } from '../display/orientation.ts'
import { ST7735_DEFAULT_PANEL, isKnownPanel, resolvePanelGeometry } from '../display/panels.ts'
import {
  defaultSt7735Layout,
  encodeSt7735Layout,
  normalizeSt7735Layout,
  type St7735LayoutDraft,
  type St7735WidgetNormalizationOptions,
} from './layout.ts'
import { normalizePin, PIN_UNSET } from '../shared/pin.ts'

export interface St7735ConfigDraft extends BaseDeviceConfig, DisplayBaseConfig {
  spiBusDeviceId: number
  chipSelectPin: number
  dcPin: number
  resetPin: number
  rotation: number
  layout: St7735LayoutDraft
}

export interface St7735CreateDraft extends DeviceCreateDraftBase, St7735ConfigDraft {}

function normalizeNumber(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) ? numeric : fallback
}

function dependencyDeviceIdFromDeps(deps: DeviceDependencyLink[] | undefined, role: string): number {
  return deps?.find(dep => dep.role === role)?.deviceId ?? 0
}

export class St7735Device extends BaseDevice<St7735ConfigDraft, St7735CreateDraft, Record<string, never>> {
  static readonly TYPE_ID = 9 as const
  static readonly TYPE_NAME = 'st7735' as const
  static readonly displayCapabilities: DisplayCapabilities = st7735Display.displayCapabilities
  static readonly BITMAP_DEFAULT_WIDTH = 16
  static readonly BITMAP_DEFAULT_HEIGHT = 16

  readonly typeName = St7735Device.TYPE_NAME
  readonly typeId = St7735Device.TYPE_ID
  readonly displayCapabilities = St7735Device.displayCapabilities

  static defaultConfig(): St7735ConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      name: 'New Display',
      panel: ST7735_DEFAULT_PANEL,
      width: 128,
      height: 160,
      spiBusDeviceId: 0,
      chipSelectPin: PIN_UNSET,
      dcPin: PIN_UNSET,
      resetPin: PIN_UNSET,
      rotation: 0,
      layout: defaultSt7735Layout(),
    }
  }

  // `layoutOptions.resolveText` lets a caller with live metric data (the display designer) size
  // auto-size text widgets against the resolved placeholder value instead of the raw
  // `{{dev.id.metric}}` token; omitted everywhere else (store hydration, mocks, tests).
  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[], layoutOptions?: St7735WidgetNormalizationOptions): St7735ConfigDraft {
    const defaults = St7735Device.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return {
        ...defaults,
        deps: Array.isArray(deps) ? deps : defaults.deps,
        spiBusDeviceId: dependencyDeviceIdFromDeps(deps, 'spi_bus'),
      }
    }
    const raw = value as Record<string, unknown>
    // width/height are always derived from the panel (never independently settable), matching
    // the firmware's St7735DeviceConfigV5::parseJson.
    const panel = isKnownPanel('st7735', raw.panel) ? (raw.panel as string) : defaults.panel
    const geometry = resolvePanelGeometry('st7735', panel)
    return {
      ...normalizeBaseDeviceConfig(raw, defaults),
      panel,
      width: geometry?.width ?? defaults.width,
      height: geometry?.height ?? defaults.height,
      spiBusDeviceId: normalizeNumber(raw.spiBusDeviceId ?? dependencyDeviceIdFromDeps(deps, 'spi_bus'), defaults.spiBusDeviceId),
      chipSelectPin: normalizePin(raw.chipSelectPin, defaults.chipSelectPin, 'output'),
      dcPin: normalizePin(raw.dcPin, defaults.dcPin, 'output'),
      resetPin: normalizePin(raw.resetPin, defaults.resetPin, 'output'),
      rotation: normalizeDisplayRotation(raw.rotation, defaults.rotation),
      layout: normalizeSt7735Layout(raw.layout, layoutOptions),
    }
  }

  // spiBusDeviceId is intentionally excluded: it is carried via `deps`, not `config`, matching
  // how ds18b20/thermostat already exclude their own dependency-id fields from encodeConfig.
  static encodeConfig(config: St7735ConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      panel: config.panel,
      width: config.width,
      height: config.height,
      chipSelectPin: config.chipSelectPin,
      dcPin: config.dcPin,
      resetPin: config.resetPin,
      rotation: config.rotation,
      layout: encodeSt7735Layout(config.layout),
    }
  }

  static supportsBitmapFormat(format: string): format is 'rgb565' {
    return st7735Display.supportsBitmapFormat(format as never)
  }

  createDefaultConfig(): St7735ConfigDraft {
    return St7735Device.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): St7735CreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): St7735CreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): St7735ConfigDraft {
    return St7735Device.normalizeConfig(value, deps)
  }

  normalizeOutput(): Record<string, never> {
    return {}
  }

  protected override encodeConfig(config: St7735ConfigDraft): Record<string, unknown> {
    return St7735Device.encodeConfig(config)
  }

  protected override createCreateDeps(config: St7735ConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'spi_bus',
        deviceId: config.spiBusDeviceId,
      },
    ]
  }
}
