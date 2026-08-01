import type { BaseDeviceConfig, DeviceDependencyLink, DeviceRecord, BinarySensorOutputSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceRole } from '@/models/device-type-ids'
import { BaseDevice, defaultBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'
import { isValidBoardPin, normalizePin } from './shared/pin.ts'

export type BinarySensorPullMode = 'none' | 'pullup' | 'pulldown'

export interface BinarySensorConfigDraft extends BaseDeviceConfig {
  gpioPin: number
  pullMode: BinarySensorPullMode
  inverted: boolean
  debounceMs: number
}

export interface BinarySensorCreateDraft extends DeviceCreateDraftBase, BinarySensorConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizePullMode(value: unknown, fallback: BinarySensorPullMode): BinarySensorPullMode {
  return value === 'none' || value === 'pullup' || value === 'pulldown' ? value : fallback
}

function normalizeDebounceMs(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 0 && numeric <= 60000 ? Math.round(numeric) : fallback
}

// GPIO 34-39 are input-only pins without internal pull resistors on the ESP32 -- exactly the
// pins that lack output capability in the board table (mirrors binarySensorPinSupportsPull() in
// the firmware, which uses the same equivalence).
export function binarySensorPinSupportsPull(pin: number): boolean {
  return isValidBoardPin(pin, 'output')
}

export class BinarySensorDevice extends BaseDevice<BinarySensorConfigDraft, BinarySensorCreateDraft, BinarySensorOutputSnapshot> {
  static readonly TYPE_ID = 18 as const
  static readonly TYPE_NAME = 'binary_sensor' as const

  readonly typeName = BinarySensorDevice.TYPE_NAME
  readonly typeId = BinarySensorDevice.TYPE_ID
  // A debounced digital input is a boolean status source: auto_switch conditions and the dosing
  // pump's low-level sensor consume it via DeviceRole::Condition.
  readonly dependencyRoles: DeviceRole[] = ['condition']

  static defaultConfig(): BinarySensorConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      gpioPin: 255,
      pullMode: 'pullup',
      inverted: false,
      debounceMs: 50,
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): BinarySensorConfigDraft {
    const defaults = BinarySensorDevice.defaultConfig()
    if (!isRecord(value)) {
      return { ...defaults, deps: Array.isArray(deps) ? deps : defaults.deps }
    }
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      gpioPin: normalizePin(value.gpioPin, defaults.gpioPin, 'input'),
      pullMode: normalizePullMode(value.pullMode, defaults.pullMode),
      inverted: value.inverted === true,
      debounceMs: normalizeDebounceMs(value.debounceMs, defaults.debounceMs),
    }
  }

  createDefaultConfig(): BinarySensorConfigDraft {
    return BinarySensorDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): BinarySensorCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): BinarySensorCreateDraft {
    return {
      ...this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): BinarySensorConfigDraft {
    return BinarySensorDevice.normalizeConfig(value, deps)
  }

  normalizeOutput(record: DeviceRecord): BinarySensorOutputSnapshot {
    return ((record.runtime as { output?: BinarySensorOutputSnapshot }).output ?? {}) as BinarySensorOutputSnapshot
  }
}
