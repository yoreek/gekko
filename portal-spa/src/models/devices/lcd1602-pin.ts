import type { DeviceRecord, DeviceDependencyLink, BaseDeviceConfig, Lcd1602PinOutputSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import { defaultLcd1602Layout, encodeLcd1602Layout, normalizeLcd1602Layout, type Lcd1602LayoutDraft } from './lcd1602/layout.ts'

// 255 is the firmware's "unset" marker for direct-GPIO HD44780 pins (kHd44780PinUnset), same
// convention as TM1637_UNSET_PIN.
export const LCD1602_PIN_UNSET = 255

export interface Lcd1602PinConfigDraft extends BaseDeviceConfig {
  rsPin: number
  ePin: number
  d4Pin: number
  d5Pin: number
  d6Pin: number
  d7Pin: number
  backlightPin: number
  layout: Lcd1602LayoutDraft
}

export interface Lcd1602PinCreateDraft extends DeviceCreateDraftBase, Lcd1602PinConfigDraft {}

function normalizePin(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isInteger(numeric) && numeric >= 0 && numeric <= 255 ? numeric : fallback
}

export class Lcd1602PinDevice extends BaseDevice<Lcd1602PinConfigDraft, Lcd1602PinCreateDraft, Lcd1602PinOutputSnapshot> {
  static readonly TYPE_ID = 34 as const
  static readonly TYPE_NAME = 'lcd1602_pin' as const

  readonly typeName = Lcd1602PinDevice.TYPE_NAME
  readonly typeId = Lcd1602PinDevice.TYPE_ID

  static defaultConfig(): Lcd1602PinConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      rsPin: LCD1602_PIN_UNSET,
      ePin: LCD1602_PIN_UNSET,
      d4Pin: LCD1602_PIN_UNSET,
      d5Pin: LCD1602_PIN_UNSET,
      d6Pin: LCD1602_PIN_UNSET,
      d7Pin: LCD1602_PIN_UNSET,
      backlightPin: LCD1602_PIN_UNSET,
      layout: defaultLcd1602Layout(),
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Lcd1602PinConfigDraft {
    const defaults = Lcd1602PinDevice.defaultConfig()
    if (typeof value !== 'object' || value === null || Array.isArray(value)) {
      return {
        ...defaults,
        deps: Array.isArray(deps) ? deps : defaults.deps,
      }
    }
    const raw = value as Record<string, unknown>
    const rawDeps = Array.isArray(raw.deps)
      ? (raw.deps.filter(dep => typeof dep === 'object' && dep !== null) as DeviceDependencyLink[])
      : defaults.deps
    return {
      ...normalizeBaseDeviceConfig(raw, defaults),
      deps: rawDeps,
      rsPin: normalizePin(raw.rsPin, defaults.rsPin),
      ePin: normalizePin(raw.ePin, defaults.ePin),
      d4Pin: normalizePin(raw.d4Pin, defaults.d4Pin),
      d5Pin: normalizePin(raw.d5Pin, defaults.d5Pin),
      d6Pin: normalizePin(raw.d6Pin, defaults.d6Pin),
      d7Pin: normalizePin(raw.d7Pin, defaults.d7Pin),
      backlightPin: normalizePin(raw.backlightPin, defaults.backlightPin),
      layout: normalizeLcd1602Layout(raw.layout ?? defaults.layout),
    }
  }

  static encodeConfig(config: Lcd1602PinConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      rsPin: config.rsPin,
      ePin: config.ePin,
      d4Pin: config.d4Pin,
      d5Pin: config.d5Pin,
      d6Pin: config.d6Pin,
      d7Pin: config.d7Pin,
      backlightPin: config.backlightPin,
      layout: encodeLcd1602Layout(config.layout),
    }
  }

  createDefaultConfig(): Lcd1602PinConfigDraft {
    return Lcd1602PinDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Lcd1602PinCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): Lcd1602PinCreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Lcd1602PinConfigDraft {
    return Lcd1602PinDevice.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): Lcd1602PinOutputSnapshot {
    return record.runtime as unknown as Lcd1602PinOutputSnapshot
  }

  protected override encodeConfig(config: Lcd1602PinConfigDraft): Record<string, unknown> {
    return Lcd1602PinDevice.encodeConfig(config)
  }

  // RS/E/D4-D7/Backlight are config pins now, so the only dependencies are the metric sources
  // the firmware derives from layout placeholders - nothing for the create/edit forms to send.
  protected override createCreateDeps(): DeviceDependencyLink[] {
    return []
  }
}
