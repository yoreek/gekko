import type { BaseDeviceConfig, DeviceDependencyLink, DeviceOutputState, DeviceRecord, DeviceCreateRequest } from '@/api/contracts'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceCommandRequest } from '@/api/contracts'

export interface DeviceCreatePayload<TConfig extends BaseDeviceConfig = BaseDeviceConfig> extends DeviceCreateRequest<TConfig> {}

export abstract class BaseDevice<
  TConfig extends object = Record<string, unknown>,
  TCreateDraft extends DeviceCreateDraftBase & object = DeviceCreateDraftBase & object,
  TOutput extends object = Record<string, unknown>,
> {
  abstract readonly typeName: string
  abstract readonly typeId: number
  readonly supportedOutputStates?: DeviceOutputState[] = undefined

  abstract createDefaultConfig(): TConfig

  abstract createDefaultCreateDraft(common?: Partial<DeviceCreateDraftBase>): TCreateDraft

  abstract createEditDraft(current: DeviceRecord): TCreateDraft

  abstract normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): TConfig

  abstract normalizeOutput(record: DeviceRecord): TOutput

  abstract buildEditCommands(current: DeviceRecord, draft: TCreateDraft): DeviceCommandRequest[]

  encodeConfig(config: TConfig): Record<string, unknown> {
    return config as Record<string, unknown>
  }

  buildCreatePayload(draft: TCreateDraft): DeviceCreatePayload {
    const config = this.extractCreateConfig(draft)
    const payload: DeviceCreatePayload = {
      typeName: this.typeName,
      config: {
        ...(this.encodeConfig(config) as unknown as Record<string, unknown>),
        name: draft.name.trim(),
        enabled: draft.enabled,
        deps: this.createCreateDeps(draft),
      } as BaseDeviceConfig,
    }
    return payload
  }

  protected abstract extractCreateConfig(draft: TCreateDraft): TConfig

  protected createCreateDeps(_draft: TCreateDraft): DeviceDependencyLink[] {
    return []
  }
}
