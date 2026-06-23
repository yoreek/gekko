import type { DeviceDependencyLink, DeviceRecord } from '@/api/contracts'
import { deviceTypeIdFromName, deviceTypeName } from '@/models/device-types'
import type { DashboardDevice, DashboardEffectiveStatus, DeviceDetailSnapshot } from '@/models/device-model'
import type { DeviceCreateDraftBase } from '@/models/devices/base'
import type { DeviceCommandRequest } from '@/api/contracts'

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

export interface DeviceCreatePayload {
  type: string
  name: string
  enabled: boolean
  deps?: DeviceDependencyLink[]
  [key: string]: unknown
}

export abstract class BaseDevice<
  TConfig extends object = Record<string, unknown>,
  TCreateDraft extends DeviceCreateDraftBase & object = DeviceCreateDraftBase & object,
  TOutput extends object = Record<string, unknown>,
> {
  abstract readonly typeName: string
  abstract readonly typeId: number

  abstract createDefaultConfig(): TConfig

  abstract createDefaultCreateDraft(common?: Partial<DeviceCreateDraftBase>): TCreateDraft

  abstract createEditDraft(current: DashboardDevice): TCreateDraft

  abstract normalizeConfig(value: unknown, deps?: DeviceDependencyLink[]): TConfig

  abstract normalizeOutput(record: DeviceRecord): TOutput

  abstract buildEditCommands(current: DashboardDevice, draft: TCreateDraft): DeviceCommandRequest[]

  encodeConfig(config: TConfig): Record<string, unknown> {
    return config as Record<string, unknown>
  }

  buildCreatePayload(draft: TCreateDraft): DeviceCreatePayload {
    const payload: DeviceCreatePayload = {
      type: this.typeName,
      name: draft.name.trim(),
      ...this.encodeConfig(this.extractCreateConfig(draft)),
      enabled: draft.enabled,
    }
    const deps = this.createCreateDeps(draft)
    if (deps.length > 0) {
      payload.deps = deps
    }
    return payload
  }

  protected abstract extractCreateConfig(draft: TCreateDraft): TConfig

  protected createCreateDeps(_draft: TCreateDraft): DeviceDependencyLink[] {
    return []
  }

  normalizeDetail(record: DeviceRecord): DeviceDetailSnapshot {
    const config = (isRecord(record.config) ? record.config : this.normalizeConfig(record.config, record.deps)) as Record<
      string,
      unknown
    >
    const detail: DeviceDetailSnapshot = {
      config,
      retainedStateSupported: Boolean(record.retained_state_supported),
      retainedStartupEnabled: record.retained_startup_enabled,
      retainedStartupFallbackOutput: record.retained_startup_fallback_output,
      retainedStateInConfigPayload: record.retained_state_in_config_payload,
      scan: record.scan,
    }

    const restorePreviousState = config.restore_previous_state
    if (typeof restorePreviousState === 'boolean') {
      detail.restorePreviousState = restorePreviousState
    }

    return detail
  }

  normalize(
    record: DeviceRecord,
    registryRevision = record.registry_revision ?? 0,
    pendingPersistence = record.pending_persistence ?? false,
  ): DashboardDevice {
    const typeName = this.normalizeTypeName(record)
    const backendEffectiveStatus = record.effective_status ?? record.lifecycle_status ?? record.status ?? 'unknown'
    const deps = this.normalizeDeps(record)
    const typeId = typeof record.type_id === 'number' ? record.type_id : deviceTypeIdFromName(typeName) || this.typeId
    const deviceId = typeof record.id === 'number' ? record.id : typeof record.device_id === 'number' ? record.device_id : 0

    return {
      deviceId,
      typeId,
      typeName,
      typeLabel: this.normalizeTypeLabel(record, typeName),
      name: record.name,
      enabled: record.enabled,
      deps,
      hasDeps: deps.length > 0,
      configVersion: record.config_version,
      configRevision: record.config_revision,
      registryRevision,
      pendingPersistence,
      lifecycleStatus: record.lifecycle_status,
      effectiveStatus: this.normalizeDashboardStatus(backendEffectiveStatus),
      backendEffectiveStatus,
      status: record.status ?? backendEffectiveStatus,
      isReady: backendEffectiveStatus === 'ready',
      detail: this.normalizeDetail(record),
      output: this.normalizeOutput(record),
      raw: record,
    }
  }

  protected normalizeTypeName(record: DeviceRecord): string {
    if (typeof record.type === 'string' && record.type.trim().length > 0) {
      return record.type.trim().toLowerCase()
    }
    if (typeof record.type_id === 'number') {
      return deviceTypeName(record.type_id)
    }
    return this.typeName
  }

  protected normalizeTypeLabel(record: DeviceRecord, typeName: string): string {
    if (typeof record.label === 'string' && record.label.trim().length > 0) {
      return record.label.trim()
    }
    if (typeof record.type === 'string' && record.type.trim().length > 0) {
      return record.type.trim()
    }
    return typeName || 'Unknown'
  }

  protected normalizeDeps(record: DeviceRecord): DeviceDependencyLink[] {
    if (!Array.isArray(record.deps)) {
      return []
    }
    return record.deps
      .filter(dep => typeof dep === 'object' && dep !== null)
      .map(dep => ({
        role: typeof dep.role === 'string' ? dep.role : '',
        device_id: Number(dep.device_id ?? 0),
      }))
      .filter(dep => dep.role.length > 0 && Number.isInteger(dep.device_id) && dep.device_id > 0)
  }

  protected normalizeDashboardStatus(effectiveStatus: string | undefined | null): DashboardEffectiveStatus {
    return effectiveStatus === 'ready' ? 'Ready' : '!Ready'
  }
}
