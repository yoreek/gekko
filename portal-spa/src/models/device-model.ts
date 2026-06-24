import type {
  DeviceDependencyLink,
  DeviceOutputSnapshot,
  DeviceRecord,
  DeviceRegistryResponse,
} from '@/api/contracts'
import { deviceTypeIdFromName } from '@/models/device-types'

export type DashboardEffectiveStatus = 'Ready' | '!Ready'

export interface DeviceCollection<TRecord extends DeviceRecord = DeviceRecord> {
  registryRevision: number
  devices: TRecord[]
}

export interface DeviceActionPreset {
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

export function deviceRecordConfig(device: DeviceRecord): DeviceRecord['config'] {
  return device.config
}

export function deviceRecordRuntime(device: DeviceRecord): DeviceRecord['runtime'] {
  return device.runtime
}

export function deviceRecordEnabled(device: DeviceRecord): boolean {
  return typeof device.config.enabled === 'boolean' ? device.config.enabled : true
}

export function deviceRecordDeps(device: DeviceRecord): DeviceDependencyLink[] {
  return Array.isArray(device.config.deps) ? device.config.deps : []
}

export function deviceRecordOutput(device: DeviceRecord): DeviceOutputSnapshot {
  return device.runtime as DeviceOutputSnapshot
}

export function deviceRecordStatus(device: DeviceRecord): string {
  return typeof device.runtime.status === 'string' ? device.runtime.status : 'unknown'
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

export function deviceActionPresets(): DeviceActionPreset[] {
  return []
}
