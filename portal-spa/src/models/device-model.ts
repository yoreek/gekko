import type {
  DeviceDetailResponse,
  DeviceMutationResponse,
  DeviceOutputSnapshot,
  DeviceRecord,
  DeviceRegistryResponse,
  OneWireScanSnapshot,
} from '@/api/contracts'

export type DashboardEffectiveStatus = 'Ready' | '!Ready'

export interface DeviceDetailSnapshot {
  config: Record<string, unknown>
  retainedStateSupported: boolean
  restorePreviousState?: boolean
  retainedStartupEnabled?: boolean
  retainedStartupFallbackOutput?: boolean
  retainedStateInConfigPayload?: boolean
  scan?: OneWireScanSnapshot
}

export interface DashboardDevice {
  deviceId: number
  typeId: number
  typeName: string
  typeLabel: string
  name: string
  enabled: boolean
  hasParent: boolean
  parentDeviceId: number
  configVersion: number
  configRevision: number
  registryRevision: number
  pendingPersistence: boolean
  lifecycleStatus: string
  effectiveStatus: DashboardEffectiveStatus
  backendEffectiveStatus: string
  status: string
  isReady: boolean
  detail: DeviceDetailSnapshot
  output: DeviceOutputSnapshot
  raw: DeviceRecord
}

export interface DashboardDeviceCollection {
  registryRevision: number
  pendingPersistence: boolean
  devices: DashboardDevice[]
}

export interface DashboardDeviceActionPreset {
  key: string
  command: string
  labelKey: string
  payload?: string
  tone?: 'primary' | 'secondary' | 'warning' | 'error'
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeTypeName(record: DeviceRecord): string {
  return typeof record.type === 'string' ? record.type.trim().toLowerCase() : ''
}

function normalizeTypeLabel(record: DeviceRecord, typeName: string): string {
  if (typeof record.label === 'string' && record.label.trim().length > 0) {
    return record.label.trim()
  }
  if (typeof record.type === 'string' && record.type.trim().length > 0) {
    return record.type.trim()
  }
  return typeName || 'Unknown'
}

function normalizeDetail(record: DeviceRecord): DeviceDetailSnapshot {
  const config = isRecord(record.config) ? record.config : {}
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

export function normalizeDashboardStatus(effectiveStatus: string | undefined | null): DashboardEffectiveStatus {
  return effectiveStatus === 'ready' ? 'Ready' : '!Ready'
}

export function normalizeDeviceRecord(
  record: DeviceRecord,
  registryRevision = record.registry_revision ?? 0,
  pendingPersistence = record.pending_persistence ?? false,
): DashboardDevice {
  const typeName = normalizeTypeName(record)
  const backendEffectiveStatus = record.effective_status ?? record.lifecycle_status ?? 'unknown'

  return {
    deviceId: record.device_id,
    typeId: record.type_id,
    typeName,
    typeLabel: normalizeTypeLabel(record, typeName),
    name: record.name,
    enabled: record.enabled,
    hasParent: record.has_parent,
    parentDeviceId: record.parent_device_id,
    configVersion: record.config_version,
    configRevision: record.config_revision,
    registryRevision,
    pendingPersistence,
    lifecycleStatus: record.lifecycle_status,
    effectiveStatus: normalizeDashboardStatus(backendEffectiveStatus),
    backendEffectiveStatus,
    status: record.status ?? backendEffectiveStatus,
    isReady: backendEffectiveStatus === 'ready',
    detail: normalizeDetail(record),
    output: record.output ?? {},
    raw: record,
  }
}

export function normalizeDeviceCollection(payload: DeviceRegistryResponse): DashboardDeviceCollection {
  return {
    registryRevision: payload.registry_revision,
    pendingPersistence: payload.pending_persistence,
    devices: payload.devices.map(device => normalizeDeviceRecord(device, payload.registry_revision, payload.pending_persistence)),
  }
}

export function normalizeDeviceDetail(payload: DeviceDetailResponse | DeviceMutationResponse): DashboardDevice | null {
  if (!payload.device) {
    return null
  }
  return normalizeDeviceRecord(payload.device, payload.registry_revision, payload.pending_persistence)
}

export function deviceActionPresets(): DashboardDeviceActionPreset[] {
  return []
}
