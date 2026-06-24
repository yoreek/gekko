import type {
  DeviceRecord,
  DeviceRegistryResponse,
} from '@/api/contracts'
import { deviceTypeIdFromName } from '@/models/device-types'

export type DashboardEffectiveStatus = 'Ready' | '!Ready'

export interface DeviceCollection<TRecord extends DeviceRecord = DeviceRecord> {
  registryRevision: number
  devices: TRecord[]
}

export function normalizeDashboardStatus(effectiveStatus: string | undefined | null): DashboardEffectiveStatus {
  return effectiveStatus === 'ready' ? 'Ready' : '!Ready'
}

export function normalizeDeviceRecord(
  record: DeviceRecord | Record<string, unknown>,
  registryRevision = 0,
): DeviceRecord {
  void registryRevision
  return record as DeviceRecord
}

export function normalizeDeviceCollection(payload: DeviceRegistryResponse): DeviceCollection {
  return {
    registryRevision: payload.registryRevision,
    devices: payload.devices,
  }
}

export function deviceRecordId(device: DeviceRecord): number {
  return device.record.id
}

export function deviceRecordTypeName(device: DeviceRecord): string {
  return device.record.typeName
}

export function deviceRecordTypeId(device: DeviceRecord): number {
  return deviceTypeIdFromName(device.record.typeName)
}

export function deviceRecordName(device: DeviceRecord): string {
  return typeof device.config.name === 'string' ? device.config.name : ''
}

export function deviceRecordEffectiveStatus(device: DeviceRecord): string {
  return typeof device.runtime.effectiveStatus === 'string'
    ? device.runtime.effectiveStatus
    : typeof device.runtime.lifecycleStatus === 'string'
      ? device.runtime.lifecycleStatus
      : typeof device.runtime.status === 'string'
        ? device.runtime.status
        : 'unknown'
}
