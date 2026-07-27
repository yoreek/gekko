import type { DeviceRecord, DeviceDependencyLink, BaseDeviceConfig, Lcd1602OutputSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceRole } from '@/models/device-type-ids'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import { defaultLcd1602Layout, encodeLcd1602Layout, normalizeLcd1602Layout, type Lcd1602LayoutDraft } from './lcd1602/layout.ts'

// Sentinel for backlightChannel meaning "not wired" -- mirrors kLcd1602ChannelUnset in
// Lcd1602DeviceConfig.h. The one optional channel: some parallel-wired boards tie backlight
// permanently high instead of giving it a PCF857x pin.
export const LCD1602_CHANNEL_UNSET = 255

type Lcd1602WiringChannels = Pick<
  Lcd1602ConfigDraft,
  'rsChannel' | 'eChannel' | 'd4Channel' | 'd5Channel' | 'd6Channel' | 'd7Channel' | 'backlightChannel'
>

export interface Lcd1602ConfigDraft extends BaseDeviceConfig {
  expanderDeviceId: number
  rsChannel: number
  eChannel: number
  d4Channel: number
  d5Channel: number
  d6Channel: number
  d7Channel: number
  backlightChannel: number
  layout: Lcd1602LayoutDraft
}

export interface Lcd1602CreateDraft extends DeviceCreateDraftBase, Lcd1602ConfigDraft {}

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
    return LCD1602_CHANNEL_UNSET
  }
  return normalizeChannel(value, fallback)
}

// Standard PCF8574 LCM1602-IIC backpack wiring -- the near-universal convention among cheap I2C
// LCD1602 modules (RS=P0, RW=P1 unused/tied low, E=P2, Backlight=P3, D4-D7=P4-P7). Used by
// Lcd1602Fields.vue's wiring-preset select purely as a form-fill convenience; it is not a
// persisted mode of its own -- the backend only ever sees the resulting channel numbers.
export function standardLcd1602Wiring(): Lcd1602WiringChannels {
  return {
    rsChannel: 0,
    eChannel: 2,
    d4Channel: 4,
    d5Channel: 5,
    d6Channel: 6,
    d7Channel: 7,
    backlightChannel: 3,
  }
}

export class Lcd1602Device extends BaseDevice<Lcd1602ConfigDraft, Lcd1602CreateDraft, Lcd1602OutputSnapshot> {
  static readonly TYPE_ID = 28 as const
  static readonly TYPE_NAME = 'lcd1602' as const

  readonly typeName = Lcd1602Device.TYPE_NAME
  readonly typeId = Lcd1602Device.TYPE_ID
  readonly dependencyRoles: DeviceRole[] = ['port_expander']

  static defaultConfig(): Lcd1602ConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      expanderDeviceId: 0,
      ...standardLcd1602Wiring(),
      layout: defaultLcd1602Layout(),
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Lcd1602ConfigDraft {
    const defaults = Lcd1602Device.defaultConfig()
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
      layout: normalizeLcd1602Layout(raw.layout ?? defaults.layout),
    }
  }

  static encodeConfig(config: Lcd1602ConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      rsChannel: config.rsChannel,
      eChannel: config.eChannel,
      d4Channel: config.d4Channel,
      d5Channel: config.d5Channel,
      d6Channel: config.d6Channel,
      d7Channel: config.d7Channel,
      backlightChannel: config.backlightChannel,
      layout: encodeLcd1602Layout(config.layout),
    }
  }

  createDefaultConfig(): Lcd1602ConfigDraft {
    return Lcd1602Device.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Lcd1602CreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): Lcd1602CreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Lcd1602ConfigDraft {
    return Lcd1602Device.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): Lcd1602OutputSnapshot {
    return record.runtime as unknown as Lcd1602OutputSnapshot
  }

  protected override encodeConfig(config: Lcd1602ConfigDraft): Record<string, unknown> {
    return Lcd1602Device.encodeConfig(config)
  }

  protected override createCreateDeps(config: Lcd1602ConfigDraft): DeviceDependencyLink[] {
    return [
      {
        role: 'port_expander',
        deviceId: config.expanderDeviceId,
      },
    ]
  }
}
