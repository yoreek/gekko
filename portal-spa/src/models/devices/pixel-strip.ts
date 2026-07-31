import type { BaseDeviceConfig, DeviceRecord, PixelStripOutputSnapshot } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceRole } from '@/models/device-type-ids'
import { BaseDevice, defaultBaseDeviceConfig, encodeBaseDeviceConfig, normalizeBaseDeviceConfig } from './base-device.ts'

export interface PixelStripConfigDraft extends BaseDeviceConfig {
  pin: number
  pixelCount: number
  // Whether to power up at the last live brightness (retained state) or always at
  // startupBrightness -- mirrors analog_output's restorePreviousState.
  restorePreviousState: boolean
  // Percent (0-100), applied only at startup (or when no retained state is available). The
  // live, currently-applied brightness is runtime state set via the SetOutput command (see
  // PixelStripWidget's live slider) and reported in runtime.output.brightness, never this field
  // -- mirrors analog_output's startupState/currentOutputState split (docs/pixel-strip.md).
  startupBrightness: number
}

export interface PixelStripCreateDraft extends DeviceCreateDraftBase, PixelStripConfigDraft {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function clampInt(value: unknown, fallback: number, min: number, max: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) ? Math.min(max, Math.max(min, Math.round(numeric))) : fallback
}

export class PixelStripDevice extends BaseDevice<PixelStripConfigDraft, PixelStripCreateDraft, PixelStripOutputSnapshot> {
  static readonly TYPE_ID = 36 as const
  static readonly TYPE_NAME = 'pixel_strip' as const

  readonly typeName = PixelStripDevice.TYPE_NAME
  readonly typeId = PixelStripDevice.TYPE_ID
  // pixel_strip provides the PixelStrip role that pixel_effect_* decorators depend on, mirroring
  // AnalogOutputDevice's dependencyRoles = ['analog_output'].
  readonly dependencyRoles: DeviceRole[] = ['pixel_strip']

  static defaultConfig(): PixelStripConfigDraft {
    return {
      ...defaultBaseDeviceConfig(),
      pin: 4,
      pixelCount: 30,
      restorePreviousState: false,
      startupBrightness: 50,
    }
  }

  static normalizeConfig(value: unknown): PixelStripConfigDraft {
    const defaults = PixelStripDevice.defaultConfig()
    if (!isRecord(value)) return defaults
    return {
      ...normalizeBaseDeviceConfig(value, defaults),
      pin: clampInt(value.pin, defaults.pin, 0, 255),
      pixelCount: clampInt(value.pixelCount, defaults.pixelCount, 1, 300),
      restorePreviousState: typeof value.restorePreviousState === 'boolean' ? value.restorePreviousState : defaults.restorePreviousState,
      startupBrightness: clampInt(value.startupBrightness, defaults.startupBrightness, 0, 100),
    }
  }

  createDefaultConfig(): PixelStripConfigDraft {
    return PixelStripDevice.defaultConfig()
  }

  createDefaultCreateDraft(common: Partial<DeviceCreateDraftBase> = {}): PixelStripCreateDraft {
    return {
      ...this.createDefaultConfig(),
      ...common,
      typeName: common.typeName ?? this.typeName,
    }
  }

  createEditDraft(current: DeviceRecord): PixelStripCreateDraft {
    return {
      ...this.normalizeConfig(current.config),
      typeName: this.typeName,
    }
  }

  normalizeConfig(value: unknown): PixelStripConfigDraft {
    return PixelStripDevice.normalizeConfig(value)
  }

  normalizeOutput(record: DeviceRecord): PixelStripOutputSnapshot {
    return (record.runtime as typeof record.runtime & { output?: PixelStripOutputSnapshot }).output ?? {}
  }

  protected override encodeConfig(config: PixelStripConfigDraft): Record<string, unknown> {
    return {
      ...encodeBaseDeviceConfig(config),
      pin: config.pin,
      pixelCount: config.pixelCount,
      restorePreviousState: config.restorePreviousState,
      startupBrightness: config.startupBrightness,
    }
  }
}
