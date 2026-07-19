import type { AnalogOutputOutputSnapshot, BaseDeviceConfig, DeviceRecord } from '@/api/contracts'
import type { DeviceRole } from '@/models/device-type-ids'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import { BaseDevice, defaultBaseDeviceConfig, encodeBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'

export interface AnalogOutputConfigDraft extends BaseDeviceConfig {
  restorePreviousState: boolean
  startupState: number
  safeState: number
  pin: number
  ledcChannel: number
  frequencyHz: number
  dutyBits: number
  inverted: boolean
}

export interface AnalogOutputCreateDraft extends DeviceCreateDraftBase, AnalogOutputConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeNumber(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) ? numeric : fallback
}

function normalizePercent(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) && numeric >= 0 && numeric <= 100 ? Math.round(numeric) : fallback
}

export class AnalogOutputDevice extends BaseDevice<AnalogOutputConfigDraft, AnalogOutputCreateDraft, AnalogOutputOutputSnapshot> {
  static readonly TYPE_ID = 20 as const
  static readonly TYPE_NAME = 'analog_output' as const

  readonly typeName = AnalogOutputDevice.TYPE_NAME
  readonly typeId = AnalogOutputDevice.TYPE_ID
  readonly dependencyRoles: DeviceRole[] = ['analog_output']

  static defaultConfig(): AnalogOutputConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      restorePreviousState: false,
      startupState: 0,
      safeState: 0,
      pin: 4,
      ledcChannel: 0,
      frequencyHz: 5000,
      dutyBits: 12,
      inverted: false,
    }
  }

  static normalizeConfig(value: unknown, _deps?: unknown): AnalogOutputConfigDraft {
    const defaults = AnalogOutputDevice.defaultConfig()
    if (!isRecord(value)) {
      return defaults
    }
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      restorePreviousState:
        typeof value.restorePreviousState === 'boolean' ? value.restorePreviousState : defaults.restorePreviousState,
      startupState: normalizePercent(value.startupState, defaults.startupState),
      safeState: normalizePercent(value.safeState, defaults.safeState),
      pin: Math.round(normalizeNumber(value.pin, defaults.pin)),
      ledcChannel: Math.round(normalizeNumber(value.ledcChannel, defaults.ledcChannel)),
      frequencyHz: Math.max(1, Math.round(normalizeNumber(value.frequencyHz, defaults.frequencyHz))),
      dutyBits: Math.max(1, Math.round(normalizeNumber(value.dutyBits, defaults.dutyBits))),
      inverted: typeof value.inverted === 'boolean' ? value.inverted : defaults.inverted,
    }
  }

  createDefaultConfig(): AnalogOutputConfigDraft {
    return AnalogOutputDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): AnalogOutputCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): AnalogOutputCreateDraft {
    return {
      ...this.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown, _deps?: unknown): AnalogOutputConfigDraft {
    return AnalogOutputDevice.normalizeConfig(value)
  }

  normalizeOutput(record: DeviceRecord): AnalogOutputOutputSnapshot {
    return ((record.runtime as { output?: AnalogOutputOutputSnapshot }).output ?? {}) as AnalogOutputOutputSnapshot
  }

  protected override encodeConfig(config: AnalogOutputConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      restorePreviousState: config.restorePreviousState,
      startupState: config.startupState,
      safeState: config.safeState,
      pin: config.pin,
      ledcChannel: config.ledcChannel,
      frequencyHz: config.frequencyHz,
      dutyBits: config.dutyBits,
      inverted: config.inverted,
    }
  }
}
