import type { BaseDeviceConfig, DeviceDependencyLink, DeviceRecord } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import { defaultTm1637Layout, encodeTm1637Layout, normalizeTm1637Layout, type Tm1637LayoutDraft } from './tm1637/layout.ts'

export const TM1637_PANEL = 'four_digit_decimal_036'

export interface Tm1637ConfigDraft extends BaseDeviceConfig {
  panel: string
  brightness: number
  rotation: 0 | 180
  clockSwitchDeviceId: number
  dataSwitchDeviceId: number
  layout: Tm1637LayoutDraft
}

export interface Tm1637CreateDraft extends DeviceCreateDraftBase, Tm1637ConfigDraft {}

function normalizeDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

function normalizeBrightness(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 0 && numeric <= 7 ? numeric : fallback
}

function normalizeRotation(value: unknown, fallback: 0 | 180): 0 | 180 {
  return value === 180 || value === '180' ? 180 : fallback
}

function deviceIdFromDeps(deps: DeviceDependencyLink[] | undefined, index: number): number {
  return deps?.[index]?.deviceId ?? 0
}

function normalizePanel(value: unknown): string {
  void value
  return TM1637_PANEL
}

export class Tm1637Device extends BaseDevice<Tm1637ConfigDraft, Tm1637CreateDraft, Record<string, never>> {
  static readonly TYPE_ID = 33 as const
  static readonly TYPE_NAME = 'tm1637' as const

  readonly typeName = Tm1637Device.TYPE_NAME
  readonly typeId = Tm1637Device.TYPE_ID

  static defaultConfig(): Tm1637ConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      panel: TM1637_PANEL,
      brightness: 7,
      rotation: 0,
      clockSwitchDeviceId: 0,
      dataSwitchDeviceId: 0,
      layout: defaultTm1637Layout(),
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Tm1637ConfigDraft {
    const defaults = Tm1637Device.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return {
        ...defaults,
        deps: Array.isArray(deps) ? deps : defaults.deps,
        clockSwitchDeviceId: deviceIdFromDeps(deps, 0),
        dataSwitchDeviceId: deviceIdFromDeps(deps, 1),
      }
    }
    const raw = value as Record<string, unknown>
    const rawDeps = Array.isArray(raw.deps)
      ? (raw.deps.filter(dep => typeof dep === 'object' && dep !== null) as DeviceDependencyLink[])
      : defaults.deps
    return {
      ...normalizeBaseDeviceConfig(raw, defaults),
      deps: rawDeps,
      panel: normalizePanel(raw.panel),
      brightness: normalizeBrightness(raw.brightness, defaults.brightness),
      rotation: normalizeRotation(raw.rotation, defaults.rotation),
      clockSwitchDeviceId: normalizeDeviceId(raw.clockSwitchDeviceId ?? deviceIdFromDeps(deps ?? rawDeps, 0)),
      dataSwitchDeviceId: normalizeDeviceId(raw.dataSwitchDeviceId ?? deviceIdFromDeps(deps ?? rawDeps, 1)),
      layout: normalizeTm1637Layout(raw.layout ?? defaults.layout),
    }
  }

  static encodeConfig(config: Tm1637ConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      panel: config.panel,
      brightness: config.brightness,
      rotation: config.rotation,
      layout: encodeTm1637Layout(config.layout),
    }
  }

  createDefaultConfig(): Tm1637ConfigDraft {
    return Tm1637Device.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Tm1637CreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): Tm1637CreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Tm1637ConfigDraft {
    return Tm1637Device.normalizeConfig(value, deps)
  }

  normalizeOutput(): Record<string, never> {
    return {}
  }

  protected override encodeConfig(config: Tm1637ConfigDraft): Record<string, unknown> {
    return Tm1637Device.encodeConfig(config)
  }

  protected override createCreateDeps(config: Tm1637ConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'switch',
        deviceId: config.clockSwitchDeviceId,
      },
      {
        role: 'switch',
        deviceId: config.dataSwitchDeviceId,
      },
    ]
  }
}
