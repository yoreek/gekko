import type {
  DeviceDetailResponse,
  DeviceDependencyLink,
  DeviceMutationResponse,
  DeviceOutputSnapshot,
  DeviceRecord,
  DeviceRegistryResponse,
  OneWireScanSnapshot,
} from '@/api/contracts'
import { resolveDeviceModel } from '@/models/devices/device-model-factory'

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
  deps: DeviceDependencyLink[]
  hasDeps: boolean
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

export function normalizeDashboardStatus(effectiveStatus: string | undefined | null): DashboardEffectiveStatus {
  return effectiveStatus === 'ready' ? 'Ready' : '!Ready'
}

export function normalizeDeviceRecord(
  record: DeviceRecord,
  registryRevision = record.registry_revision ?? 0,
  pendingPersistence = record.pending_persistence ?? false,
): DashboardDevice {
  return resolveDeviceModel(record).normalize(record, registryRevision, pendingPersistence)
}

export function normalizeDeviceCollection(payload: DeviceRegistryResponse): DashboardDeviceCollection {
  return {
    registryRevision: payload.registry_revision,
    pendingPersistence: payload.pending_persistence,
    devices: payload.devices.map(device => normalizeDeviceRecord(device, payload.registry_revision, payload.pending_persistence)),
  }
}

export function deviceActionPresets(): DashboardDeviceActionPreset[] {
  return []
}
