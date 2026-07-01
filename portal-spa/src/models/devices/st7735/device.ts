import type { BaseDeviceConfig, DeviceCommandRequest, DeviceDependencyLink, DeviceRecord } from '../../../api/contracts.ts'
import type { DeviceCreateDraftBase } from '../base.ts'
import { BaseDevice } from '../base-device.ts'
import { st7735Display } from '../display/display.ts'
import type { DisplayBaseConfig, DisplayCapabilities } from '../display/base.ts'
import { normalizeDisplayRotation } from '../display/orientation.ts'
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
      enabled: true,
      name: 'New Display',
      deps: [],
      width: 128,
      height: 160,
      spiBusDeviceId: 0,
      chipSelectPin: 5,
      dcPin: 2,
      resetPin: -1,
      rotation: 0,
      layout: defaultSt7735Layout(),
    }
  }

  static normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): St7735ConfigDraft {
    const defaults = St7735Device.defaultConfig()
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
      dcPin: normalizeNumber(raw.dcPin, defaults.dcPin),
      resetPin: normalizeNumber(raw.resetPin, defaults.resetPin),
      rotation: normalizeDisplayRotation(raw.rotation, defaults.rotation) % 2,
      layout: normalizeSt7735Layout(raw.layout),
    }
  }

  static encodeConfig(config: St7735ConfigDraft): Record<string, unknown> {
    return {
      name: config.name,
      enabled: config.enabled,
      deps: config.deps,
      width: config.width,
      height: config.height,
      spiBusDeviceId: config.spiBusDeviceId,
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

  override encodeConfig(config: St7735ConfigDraft): Record<string, unknown> {
    return St7735Device.encodeConfig(config)
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
      nextConfig.dcPin !== currentConfig.dcPin ||
      nextConfig.resetPin !== currentConfig.resetPin ||
      nextConfig.rotation !== currentConfig.rotation ||
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
