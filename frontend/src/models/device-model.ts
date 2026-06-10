import type { DeviceCommandRequest, DeviceRecord, DeviceRegistryResponse, DeviceDetailResponse, DeviceMutationResponse } from '../api/contracts'
import { DUMMY_DEVICE_TYPE_ID } from './device-types'

export interface DeviceDetailSnapshot {
  config: Record<string, unknown>
  retainedStateSupported: boolean
  outputState?: boolean
  restorePreviousState?: boolean
  retainedStartupEnabled?: boolean
  retainedStartupFallbackOutput?: boolean
  retainedStateInConfigPayload?: boolean
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
  effectiveStatus: string
  status: string
  detail: DeviceDetailSnapshot
  raw: DeviceRecord
}

export interface DashboardDeviceCollection {
  registryRevision: number
  pendingPersistence: boolean
  devices: DashboardDevice[]
}

export interface DashboardDeviceActionPreset {
  key: string
  command: DeviceCommandRequest['command']
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
  return typeName
}

function normalizeDetail(record: DeviceRecord, typeName: string): DeviceDetailSnapshot {
  const config = isRecord(record.config) ? record.config : {}
  const detail: DeviceDetailSnapshot = {
    config,
    retainedStateSupported: Boolean(record.retained_state_supported),
    retainedStartupEnabled: record.retained_startup_enabled,
    retainedStartupFallbackOutput: record.retained_startup_fallback_output,
    retainedStateInConfigPayload: record.retained_state_in_config_payload,
  }

  if (record.type_id === DUMMY_DEVICE_TYPE_ID) {
    const outputState = config.current_output
    const restorePreviousState = config.restore_previous_state
    if (typeof outputState === 'boolean') {
      detail.outputState = outputState
    }
    if (typeof restorePreviousState === 'boolean') {
      detail.restorePreviousState = restorePreviousState
    }
  }

  return detail
}

export function normalizeDeviceRecord(record: DeviceRecord, registryRevision = record.registry_revision ?? 0, pendingPersistence = record.pending_persistence ?? false): DashboardDevice {
  const typeName = normalizeTypeName(record)
  const effectiveStatus = record.effective_status ?? record.lifecycle_status ?? 'unknown'
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
    effectiveStatus,
    status: record.status ?? effectiveStatus,
    detail: normalizeDetail(record, typeName),
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

export function deviceActionPresets(device: DashboardDevice): DashboardDeviceActionPreset[] {
  if (device.typeId !== DUMMY_DEVICE_TYPE_ID) {
    return []
  }

  return [
    { key: 'output-on', command: 'custom', labelKey: 'device.commands.outputOn', payload: 'output=1', tone: 'primary' },
    { key: 'output-off', command: 'custom', labelKey: 'device.commands.outputOff', payload: 'output=0', tone: 'secondary' },
    { key: 'fault', command: 'set_status', labelKey: 'device.commands.fault', payload: 'fault', tone: 'warning' },
    { key: 'ready', command: 'set_status', labelKey: 'device.commands.ready', payload: 'ready', tone: 'secondary' },
  ]
}
