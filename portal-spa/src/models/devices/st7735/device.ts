import type { BaseDeviceConfig, DeviceCommandRequest, DeviceDependencyLink, DeviceRecord } from '../../../api/contracts.ts'
import type { DeviceCreateDraftBase } from '../base.ts'
import { BaseDevice } from '../base-device.ts'
import { ST7735_DEVICE_TYPE_ID } from '../../device-type-ids.ts'
import { st7735Display } from '../display/display.ts'
import type { DisplayBaseConfig, DisplayCapabilities } from '../display/base.ts'
import { st7735LayoutChanged } from './layout.ts'
import {
  defaultSt7735Layout,
  encodeSt7735Layout,
  normalizeSt7735Layout,
  type St7735LayoutDraft,
} from './layout.ts'

export interface St7735ConfigDraft extends BaseDeviceConfig, DisplayBaseConfig {
  spiBusDeviceId: number
  chipSelectPin: number
  layout: St7735LayoutDraft
}

export interface St7735CreateDraft extends DeviceCreateDraftBase, St7735ConfigDraft {}

export const displayCapabilities: DisplayCapabilities = st7735Display.displayCapabilities

export const ST7735_BITMAP_DEFAULT_WIDTH = 16
export const ST7735_BITMAP_DEFAULT_HEIGHT = 16

function normalizeNumber(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) ? numeric : fallback
}

function dependencyDeviceIdFromDeps(deps: DeviceDependencyLink[] | undefined, role: string): number {
  return deps?.find(dep => dep.role === role)?.deviceId ?? 0
}

export function defaultConfig(): St7735ConfigDraft {
  return {
    enabled: true,
    name: 'New Display',
    deps: [],
    width: 128,
    height: 160,
    spiBusDeviceId: 0,
    chipSelectPin: 5,
    layout: defaultSt7735Layout(),
  }
}

export function normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): St7735ConfigDraft {
  const defaults = defaultConfig()
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return {
      ...defaults,
      deps: Array.isArray(deps) ? deps : defaults.deps,
      spiBusDeviceId: dependencyDeviceIdFromDeps(deps, 'spi_bus'),
    }
  }
  const raw = value as Record<string, unknown>
  const nextDeps = Array.isArray(raw.deps) ? (raw.deps as DeviceDependencyLink[]) : defaults.deps
  return {
    ...defaults,
    name: typeof raw.name === 'string' ? raw.name : defaults.name,
    enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
    deps: nextDeps,
    width: normalizeNumber(raw.width, defaults.width),
    height: normalizeNumber(raw.height, defaults.height),
    spiBusDeviceId: normalizeNumber(raw.spiBusDeviceId ?? dependencyDeviceIdFromDeps(deps, 'spi_bus'), defaults.spiBusDeviceId),
    chipSelectPin: normalizeNumber(raw.chipSelectPin, defaults.chipSelectPin),
    layout: normalizeSt7735Layout(raw.layout),
  }
}

export function encodeConfig(config: St7735ConfigDraft): Record<string, unknown> {
  return {
    name: config.name,
    enabled: config.enabled,
    deps: config.deps,
    width: config.width,
    height: config.height,
    spiBusDeviceId: config.spiBusDeviceId,
    chipSelectPin: config.chipSelectPin,
    layout: encodeSt7735Layout(config.layout),
  }
}

export class Device extends BaseDevice<St7735ConfigDraft, St7735CreateDraft, Record<string, never>> {
  readonly typeName = 'st7735'
  readonly typeId = ST7735_DEVICE_TYPE_ID
  readonly displayCapabilities = displayCapabilities

  createDefaultConfig(): St7735ConfigDraft {
    return defaultConfig()
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
    return normalizeConfig(value, deps)
  }

  normalizeOutput(): Record<string, never> {
    return {}
  }

  override encodeConfig(config: St7735ConfigDraft): Record<string, unknown> {
    return encodeConfig(config)
  }

  buildEditCommands(current: DeviceRecord, draft: St7735CreateDraft): DeviceCommandRequest[] {
    const currentConfig = this.normalizeConfig(current.config, current.config.deps as DeviceDependencyLink[] | undefined)
    const commands: DeviceCommandRequest[] = []
    if (draft.name.trim() !== currentConfig.name) {
      commands.push({ command: 'rename', name: draft.name.trim() })
    }
    if (draft.enabled !== currentConfig.enabled) {
      commands.push({ command: draft.enabled ? 'enable' : 'disable' })
    }
    const nextConfig = this.normalizeConfig(draft, [
      {
        role: 'spi_bus',
        deviceId: draft.spiBusDeviceId,
      },
    ])
    if (
      nextConfig.width !== currentConfig.width ||
      nextConfig.height !== currentConfig.height ||
      nextConfig.spiBusDeviceId !== currentConfig.spiBusDeviceId ||
      nextConfig.chipSelectPin !== currentConfig.chipSelectPin ||
      st7735LayoutChanged(nextConfig.layout, currentConfig.layout)
    ) {
      commands.push({
        command: 'updateConfig',
        config: {
          ...this.encodeConfig(nextConfig),
          enabled: draft.enabled,
        },
        deps: [
          {
            role: 'spi_bus',
            deviceId: nextConfig.spiBusDeviceId,
          },
        ],
      })
    }
    return commands
  }

  protected extractCreateConfig(draft: St7735CreateDraft): St7735ConfigDraft {
    return { ...draft }
  }

  protected override createCreateDeps(config: St7735ConfigDraft) {
    return [
      {
        role: 'spi_bus',
        deviceId: config.spiBusDeviceId,
      },
    ]
  }
}

export function supportsBitmapFormat(format: string): format is 'rgb565' {
  return st7735Display.supportsBitmapFormat(format as never)
}
