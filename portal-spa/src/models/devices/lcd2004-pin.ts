import type { DeviceRecord, DeviceDependencyLink, BaseDeviceConfig, Lcd2004PinOutputSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig, encodeBaseDeviceConfig } from './base-device.ts'
import { defaultLcd2004Layout, encodeLcd2004Layout, normalizeLcd2004Layout, type Lcd2004LayoutDraft } from './lcd2004/layout.ts'
import { normalizePin } from './shared/pin.ts'

// 255 is the firmware's "unset" marker for direct-GPIO HD44780 pins (kHd44780PinUnset), same
// convention as TM1637_UNSET_PIN / LCD1602_PIN_UNSET.
export const LCD2004_PIN_UNSET = 255

export interface Lcd2004PinConfigDraft extends BaseDeviceConfig {
  rsPin: number
  ePin: number
  d4Pin: number
  d5Pin: number
  d6Pin: number
  d7Pin: number
  backlightPin: number
  layout: Lcd2004LayoutDraft
}

export interface Lcd2004PinCreateDraft extends DeviceCreateDraftBase, Lcd2004PinConfigDraft {}

export class Lcd2004PinDevice extends BaseDevice<Lcd2004PinConfigDraft, Lcd2004PinCreateDraft, Lcd2004PinOutputSnapshot> {
  static readonly TYPE_ID = 35 as const
  static readonly TYPE_NAME = 'lcd2004_pin' as const

  readonly typeName = Lcd2004PinDevice.TYPE_NAME
  readonly typeId = Lcd2004PinDevice.TYPE_ID

  static defaultConfig(): Lcd2004PinConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      rsPin: LCD2004_PIN_UNSET,
      ePin: LCD2004_PIN_UNSET,
      d4Pin: LCD2004_PIN_UNSET,
      d5Pin: LCD2004_PIN_UNSET,
      d6Pin: LCD2004_PIN_UNSET,
      d7Pin: LCD2004_PIN_UNSET,
      backlightPin: LCD2004_PIN_UNSET,
      layout: defaultLcd2004Layout(),
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Lcd2004PinConfigDraft {
    const defaults = Lcd2004PinDevice.defaultConfig()
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
      rsPin: normalizePin(raw.rsPin, defaults.rsPin, 'output'),
      ePin: normalizePin(raw.ePin, defaults.ePin, 'output'),
      d4Pin: normalizePin(raw.d4Pin, defaults.d4Pin, 'output'),
      d5Pin: normalizePin(raw.d5Pin, defaults.d5Pin, 'output'),
      d6Pin: normalizePin(raw.d6Pin, defaults.d6Pin, 'output'),
      d7Pin: normalizePin(raw.d7Pin, defaults.d7Pin, 'output'),
      backlightPin: normalizePin(raw.backlightPin, defaults.backlightPin, 'output'),
      layout: normalizeLcd2004Layout(raw.layout ?? defaults.layout),
    }
  }

  static encodeConfig(config: Lcd2004PinConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      rsPin: config.rsPin,
      ePin: config.ePin,
      d4Pin: config.d4Pin,
      d5Pin: config.d5Pin,
      d6Pin: config.d6Pin,
      d7Pin: config.d7Pin,
      backlightPin: config.backlightPin,
      layout: encodeLcd2004Layout(config.layout),
    }
  }

  createDefaultConfig(): Lcd2004PinConfigDraft {
    return Lcd2004PinDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): Lcd2004PinCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): Lcd2004PinCreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): Lcd2004PinConfigDraft {
    return Lcd2004PinDevice.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): Lcd2004PinOutputSnapshot {
    return record.runtime as unknown as Lcd2004PinOutputSnapshot
  }

  protected override encodeConfig(config: Lcd2004PinConfigDraft): Record<string, unknown> {
    return Lcd2004PinDevice.encodeConfig(config)
  }

  // RS/E/D4-D7/Backlight are config pins now, so the only dependencies are the metric sources
  // the firmware derives from layout placeholders - nothing for the create/edit forms to send.
  protected override createCreateDeps(): DeviceDependencyLink[] {
    return []
  }
}
