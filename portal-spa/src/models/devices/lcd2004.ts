import type { DeviceRecord, DeviceDependencyLink, BaseDeviceConfig, Lcd2004OutputSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceRole } from '@/models/device-type-ids'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import { defaultLcd2004Layout, encodeLcd2004Layout, normalizeLcd2004Layout, type Lcd2004LayoutDraft } from './lcd2004/layout.ts'
import { standardLcd1602Wiring } from './lcd1602.ts'

export const LCD2004_CHANNEL_UNSET = 255

type Lcd2004WiringChannels = Pick<
  Lcd2004ConfigDraft,
  'rsChannel' | 'eChannel' | 'd4Channel' | 'd5Channel' | 'd6Channel' | 'd7Channel' | 'backlightChannel'
>

export interface Lcd2004ConfigDraft extends BaseDeviceConfig {
  expanderDeviceId: number
  rsChannel: number
  eChannel: number
  d4Channel: number
  d5Channel: number
  d6Channel: number
  d7Channel: number
  backlightChannel: number
  layout: Lcd2004LayoutDraft
}

export interface Lcd2004CreateDraft extends DeviceCreateDraftBase, Lcd2004ConfigDraft {}

function deviceIdFromDeps(deps: DeviceDependencyLink[] | undefined, role: DeviceRole): number {
  return deps?.find(dep => dep.role === role)?.deviceId ?? 0
}

function normalizeDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric > 0 ? numeric : 0
}

function normalizeChannel(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 0 && numeric <= 15 ? numeric : fallback
}

function normalizeBacklightChannel(value: unknown, fallback: number): number {
  if (value === null) {
    return LCD2004_CHANNEL_UNSET
  }
  return normalizeChannel(value, fallback)
}

// Same near-universal PCF8574 LCM-IIC backpack wiring as lcd1602 (RS=P0, RW=P1 unused/tied low,
// E=P2, Backlight=P3, D4-D7=P4-P7) -- the channel roles are identical, only the visible geometry
// (columns/rows) differs between the two panel types.
export function standardLcd2004Wiring(): Lcd2004WiringChannels {
  return standardLcd1602Wiring()
}

export class Lcd2004Device extends BaseDevice<Lcd2004ConfigDraft, Lcd2004CreateDraft, Lcd2004OutputSnapshot> {
  static readonly TYPE_ID = 30 as const
  static readonly TYPE_NAME = 'lcd2004' as const

  readonly typeName = Lcd2004Device.TYPE_NAME
  readonly typeId = Lcd2004Device.TYPE_ID
  readonly dependencyRoles: DeviceRole[] = ['port_expander']

  static defaultConfig(): Lcd2004ConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      expanderDeviceId: 0,
      ...standardLcd2004Wiring(),
      layout: defaultLcd2004Layout(),
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Lcd2004ConfigDraft {
    const defaults = Lcd2004Device.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return {
        ...defaults,
        deps: Array.isArray(deps) ? deps : defaults.deps,
        expanderDeviceId: deviceIdFromDeps(deps, 'port_expander'),
      }
    }
    const raw = value as Record<string, unknown>
    const rawDeps = Array.isArray(raw.deps)
      ? (raw.deps.filter(dep => typeof dep === 'object' && dep !== null) as DeviceDependencyLink[])
      : defaults.deps
    return {
      ...normalizeBaseDeviceConfig(raw, defaults),
      deps: rawDeps,
      expanderDeviceId: normalizeDeviceId(raw.expanderDeviceId ?? deviceIdFromDeps(deps ?? rawDeps, 'port_expander')),
      rsChannel: normalizeChannel(raw.rsChannel, defaults.rsChannel),
      eChannel: normalizeChannel(raw.eChannel, defaults.eChannel),
      d4Channel: normalizeChannel(raw.d4Channel, defaults.d4Channel),
      d5Channel: normalizeChannel(raw.d5Channel, defaults.d5Channel),
      d6Channel: normalizeChannel(raw.d6Channel, defaults.d6Channel),
      d7Channel: normalizeChannel(raw.d7Channel, defaults.d7Channel),
      backlightChannel: normalizeBacklightChannel(raw.backlightChannel, defaults.backlightChannel),
      layout: normalizeLcd2004Layout(raw.layout ?? defaults.layout),
    }
  }

  static encodeConfig(config: Lcd2004ConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      rsChannel: config.rsChannel,
      eChannel: config.eChannel,
      d4Channel: config.d4Channel,
      d5Channel: config.d5Channel,
      d6Channel: config.d6Channel,
      d7Channel: config.d7Channel,
      backlightChannel: config.backlightChannel,
      layout: encodeLcd2004Layout(config.layout),
    }
  }

  createDefaultConfig(): Lcd2004ConfigDraft {
    return Lcd2004Device.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Lcd2004CreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): Lcd2004CreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Lcd2004ConfigDraft {
    return Lcd2004Device.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): Lcd2004OutputSnapshot {
    return record.runtime as unknown as Lcd2004OutputSnapshot
  }

  protected override encodeConfig(config: Lcd2004ConfigDraft): Record<string, unknown> {
    return Lcd2004Device.encodeConfig(config)
  }

  protected override createCreateDeps(config: Lcd2004ConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'port_expander',
        deviceId: config.expanderDeviceId,
      },
    ]
  }
}
