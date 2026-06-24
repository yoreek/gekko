import type {
  DeviceDetailResponse,
  DeviceDependencyLink,
  DeviceMutationResponse,
  DeviceOutputSnapshot,
  DeviceRecord,
  DeviceRegistryResponse,
  OneWireScanSnapshot,
} from '@/api/contracts'
import { deviceTypeIdFromName, deviceTypeName } from '@/models/device-types'

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

export interface DashboardDevice extends DeviceRecord {
  deviceId: number
  typeId: number
  typeLabel: string
  name: string
  enabled: boolean
  deps: DeviceDependencyLink[]
  hasDeps: boolean
  configRevision: number
  registryRevision: number
  lifecycleStatus: string
  effectiveStatus: DashboardEffectiveStatus
  backendEffectiveStatus: string
  status: string
  isReady: boolean
  detail: DeviceDetailSnapshot
  output: DeviceOutputSnapshot
}

export interface DashboardDeviceCollection<TRecord extends DashboardDevice = DashboardDevice> {
  registryRevision: number
  devices: TRecord[]
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

function readRecord(source: DeviceRecord | Record<string, unknown>): DeviceRecord {
  return source as DeviceRecord
}

function readConfig(source: DeviceRecord): Record<string, unknown> {
  return (source.config ?? {}) as unknown as Record<string, unknown>
}

function readRuntime(source: DeviceRecord): Record<string, unknown> {
  return (source.runtime ?? {}) as unknown as Record<string, unknown>
}

function readDeps(source: DeviceRecord): DeviceDependencyLink[] {
  const config = readConfig(source)
  return Array.isArray(config.deps) ? (config.deps as DeviceDependencyLink[]) : []
}

function buildDetail(source: DeviceRecord): DeviceDetailSnapshot {
  const config = readConfig(source)
  const runtime = readRuntime(source)
  const detail: DeviceDetailSnapshot = {
    config,
    retainedStateSupported: Boolean((config as { retainedStateSupported?: unknown }).retainedStateSupported ?? false),
    retainedStartupEnabled: (config as { retainedStartupEnabled?: boolean }).retainedStartupEnabled,
    retainedStartupFallbackOutput: (config as { retainedStartupFallbackOutput?: boolean }).retainedStartupFallbackOutput,
    retainedStateInConfigPayload: (config as { retainedStateInConfigPayload?: boolean }).retainedStateInConfigPayload,
    scan: runtime.scan as OneWireScanSnapshot | undefined,
  }
  const restorePreviousState = (config as { restorePreviousState?: unknown }).restorePreviousState
  if (typeof restorePreviousState === 'boolean') {
    detail.restorePreviousState = restorePreviousState
  }
  return detail
}

export function createDashboardDevice(
  source: DeviceRecord | Record<string, unknown>,
  registryRevision = 0,
): DashboardDevice {
  const record = readRecord(source)
  const config = readConfig(record)
  const runtime = readRuntime(record)
  const deps = readDeps(record)
  const typeName = record.record?.typeName ?? ''
  const device: DashboardDevice = {
    ...record,
    deviceId: record.record?.id ?? 0,
    typeId: deviceTypeIdFromName(typeName) || 0,
    typeLabel: typeName || deviceTypeName(deviceTypeIdFromName(typeName) || 0),
    name: typeof config.name === 'string' ? config.name : '',
    enabled: typeof config.enabled === 'boolean' ? config.enabled : true,
    deps,
    hasDeps: deps.length > 0,
    configRevision: record.record?.configRevision ?? 0,
    registryRevision,
    lifecycleStatus: typeof runtime.lifecycleStatus === 'string' ? runtime.lifecycleStatus : 'unknown',
    effectiveStatus: normalizeDashboardStatus(
      typeof runtime.effectiveStatus === 'string' ? runtime.effectiveStatus : typeof runtime.lifecycleStatus === 'string' ? runtime.lifecycleStatus : typeof runtime.status === 'string' ? runtime.status : 'unknown',
    ),
    backendEffectiveStatus:
      typeof runtime.effectiveStatus === 'string'
        ? runtime.effectiveStatus
        : typeof runtime.lifecycleStatus === 'string'
          ? runtime.lifecycleStatus
          : typeof runtime.status === 'string'
            ? runtime.status
            : 'unknown',
    status: typeof runtime.status === 'string'
      ? runtime.status
      : typeof runtime.effectiveStatus === 'string'
        ? runtime.effectiveStatus
        : typeof runtime.lifecycleStatus === 'string'
          ? runtime.lifecycleStatus
          : 'unknown',
    isReady:
      (typeof runtime.effectiveStatus === 'string' ? runtime.effectiveStatus : runtime.status) === 'ready',
    detail: buildDetail(record),
    output: (runtime.output ?? {}) as DeviceOutputSnapshot,
  }
  return device
}

export function normalizeDeviceRecord(
  record: DeviceRecord | Record<string, unknown>,
  registryRevision = 0,
): DashboardDevice {
  return createDashboardDevice(record, registryRevision)
}

export function normalizeDeviceCollection(payload: DeviceRegistryResponse): DashboardDeviceCollection {
  return {
    registryRevision: payload.registryRevision,
    devices: payload.devices.map(device => createDashboardDevice(device, payload.registryRevision)),
  }
}

export function deviceActionPresets(): DashboardDeviceActionPreset[] {
  return []
}
