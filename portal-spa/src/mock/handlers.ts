import type {
  DashboardLayoutRecord,
  DashboardLayoutResponse,
  DeviceCommandRequest,
  DeviceDetailResponse,
  DeviceMutationResponse,
  DeviceRecord,
  DeviceRegistryResponse,
  OtaStatusResponse,
  SystemRestartResponse,
  WifiScanResponse,
  WifiStatusResponse,
} from '@/api'
import { ApiClientError } from '@/api/http'
import { publishRealtimeMessage } from '@/realtime/bus'
import { scheduleMockPersistenceFlush } from '@/realtime/mockRuntime'
import { DUMMY_DEVICE_TYPE_ID } from '@/models/device-types'
import { createSeedMockDatabase, loadMockDatabase, saveMockDatabase } from './database'

function ok<T extends object>(payload: T): T & { success: true } {
  return {
    success: true,
    ...payload,
  }
}

function mutateRegistry<T>(mutator: (db: ReturnType<typeof createSeedMockDatabase>) => T): T {
  const db = loadMockDatabase()
  const result = mutator(db)
  saveMockDatabase(db)
  return result
}

export function mockFetchWifiStatus(): WifiStatusResponse {
  const db = loadMockDatabase()
  return ok({
    wifi_status: db.wifi.status,
    station_ip: db.wifi.stationIp,
    setup_ap_ip: db.wifi.setupApIp,
  })
}

export function mockFetchWifiScan(): WifiScanResponse {
  const db = loadMockDatabase()
  return ok({
    status: 'ok',
    networks: db.wifi.scan,
  })
}

export function mockConfigureWifi(ssid: string, password = ''): Promise<{ status: string }> {
  const response = mutateRegistry(db => {
      db.wifi.status = 'connecting'
      db.registryRevision += 1
      db.pendingPersistence = true
      return ok({
        status: 'accepted',
        ssid,
        password,
      })
  })
  const db = loadMockDatabase()
  publishRealtimeMessage({
    topic: 'wifi.status',
    revision: db.registryRevision,
    payload: {
      wifi_status: db.wifi.status,
      station_ip: db.wifi.stationIp,
      setup_ap_ip: db.wifi.setupApIp,
    },
  })
  scheduleMockPersistenceFlush()
  return Promise.resolve(response)
}

export function mockStartBleWifiConfig(): Promise<{ status: string; action: string }> {
  const response = mutateRegistry(db => {
    db.wifi.status = 'ble_config'
    db.registryRevision += 1
    db.pendingPersistence = true
    return ok({
      status: 'accepted',
      action: 'start_ble_config',
    })
  })
  publishRealtimeMessage({
    topic: 'wifi.status',
    revision: loadMockDatabase().registryRevision,
    payload: {
      wifi_status: 'ble_config',
      station_ip: loadMockDatabase().wifi.stationIp,
      setup_ap_ip: loadMockDatabase().wifi.setupApIp,
    },
  })
  scheduleMockPersistenceFlush()
  return Promise.resolve(response)
}

export function mockFetchDevices(): DeviceRegistryResponse {
  const db = loadMockDatabase()
  return ok({
    registry_revision: db.registryRevision,
    pending_persistence: db.pendingPersistence,
    devices: db.devices,
  })
}

export function mockFetchDevice(deviceId: number): DeviceDetailResponse {
  const db = loadMockDatabase()
  const device = db.devices.find(entry => entry.device_id === deviceId)
  if (!device) {
    throw new ApiClientError('device not found', 'NOT_FOUND', 404, null)
  }
  return ok({
    registry_revision: db.registryRevision,
    pending_persistence: db.pendingPersistence,
    device,
  })
}

export function mockFetchDashboardLayout(): DashboardLayoutResponse {
  const db = loadMockDatabase()
  pruneDashboardLayout(db)
  saveMockDatabase(db)
  return ok({
    revision: db.dashboardLayoutRevision,
    layout_defaulted: false,
    layout: db.dashboardLayout,
  })
}

export function mockSaveDashboardLayout(layout: DashboardLayoutRecord): Promise<DashboardLayoutResponse> {
  const response = mutateRegistry(db => {
    validateDashboardLayout(layout)
    db.dashboardLayout = normalizeDashboardLayout(layout, db.devices.map(device => device.device_id))
    db.dashboardLayoutRevision += 1
    return ok({
      revision: db.dashboardLayoutRevision,
      layout_defaulted: false,
      layout: db.dashboardLayout,
    })
  })
  publishRealtimeMessage({
    topic: 'dashboard.layout',
    revision: response.revision,
    payload: {
      revision: response.revision,
    },
  })
  return Promise.resolve(response)
}

export function mockCreateDevice(payload: Record<string, unknown>): Promise<DeviceMutationResponse> {
  const response = mutateRegistry(db => {
    const nextId = Math.max(1, ...db.devices.map(device => device.device_id)) + 1
    const typeId = payload.type_id
    if (typeof typeId !== 'number' || typeId !== DUMMY_DEVICE_TYPE_ID) {
      throw new ApiClientError('unsupported device type', 'UNSUPPORTED_TYPE', 400, null)
    }

    const device: DeviceRecord = {
      device_id: nextId,
      type_id: typeId,
      label: 'Dummy device',
      type: 'dummy',
      name: String(payload.name ?? 'New Device'),
      enabled: Boolean(payload.enabled ?? true),
      has_parent: false,
      parent_device_id: 0,
      config_version: 1,
      config_revision: 1,
      lifecycle_status: 'ready',
      effective_status: 'ready',
      status: 'ready',
      retained_state_supported: true,
      retained_startup_enabled: false,
      retained_startup_fallback_output: false,
      retained_state_in_config_payload: false,
      config: {
        enabled: true,
        restore_previous_state: false,
        default_output: false,
        current_output: false,
        inverted: false,
      },
    }
    if (isRecordPayload(payload.config)) {
      device.config = {
        ...device.config,
        ...payload.config,
      }
    }
    db.devices.push(device)
    db.registryRevision += 1
    db.pendingPersistence = true
    return ok({
      registry_revision: db.registryRevision,
      pending_persistence: db.pendingPersistence,
      device: db.devices.at(-1),
    })
  })
  const db = loadMockDatabase()
  publishRealtimeMessage({
    topic: 'device.upsert',
    revision: db.registryRevision,
    payload: response.device ?? {},
  })
  scheduleMockPersistenceFlush()
  return Promise.resolve(response)
}

export function mockCommandDevice(deviceId: number, payload: DeviceCommandRequest): Promise<DeviceMutationResponse> {
  const response = mutateRegistry(db => {
      const device = db.devices.find(entry => entry.device_id === deviceId)
      if (!device) {
        throw new ApiClientError('device not found', 'NOT_FOUND', 404, null)
      }

      switch (payload.command) {
        case 'rename':
          device.name = String(payload.payload ?? device.name)
          break
        case 'enable':
          device.enabled = true
          device.lifecycle_status = 'ready'
          device.effective_status = 'ready'
          device.status = 'ready'
          break
        case 'disable':
          device.enabled = false
          device.lifecycle_status = 'disabled'
          device.effective_status = 'disabled'
          device.status = 'disabled'
          break
        case 'delete':
          db.devices = db.devices.filter(entry => entry.device_id !== deviceId)
          pruneDashboardLayout(db)
          break
        case 'update_config':
          device.config_revision += 1
          break
        case 'set_status':
          if (payload.payload === 'fault') {
            device.lifecycle_status = 'faulted'
            device.effective_status = 'faulted'
            device.status = 'faulted'
          }
          if (payload.payload === 'ready') {
            device.lifecycle_status = 'ready'
            device.effective_status = 'ready'
            device.status = 'ready'
          }
          break
        case 'custom':
          if (!isRecordPayload(device.config)) {
            device.config = {}
          }
          if (payload.payload === 'output=1') {
            device.config = {
              ...device.config,
              current_output: true,
            }
          }
          if (payload.payload === 'output=0') {
            device.config = {
              ...device.config,
              current_output: false,
            }
          }
          break
        case 'set_parent':
          device.has_parent = Boolean(payload.has_parent ?? true)
          device.parent_device_id = Number(payload.parent_device_id ?? 0)
          break
        default:
          break
      }

      db.registryRevision += 1
      db.pendingPersistence = true
      return ok({
        registry_revision: db.registryRevision,
        pending_persistence: db.pendingPersistence,
        device: db.devices.find(entry => entry.device_id === deviceId),
      })
  })
  const db = loadMockDatabase()
  publishRealtimeMessage({
    topic: payload.command === 'delete' ? 'device.remove' : 'device.command_result',
    revision: db.registryRevision,
    payload: response.device ?? { device_id: deviceId },
  })
  scheduleMockPersistenceFlush()
  return Promise.resolve(response)
}

export function mockDeleteDevice(deviceId: number): Promise<DeviceMutationResponse> {
  return mockCommandDevice(deviceId, {
    command: 'delete',
  })
}

export function mockFetchOtaStatus(): OtaStatusResponse {
  const db = loadMockDatabase()
  return ok(db.ota)
}

export function mockRestartSystem(): Promise<SystemRestartResponse> {
  const response = mutateRegistry(db => {
      db.system.status = 'rebooting'
      db.system.rebooting = true
      return ok({
        rebooting: true,
      })
  })
  publishRealtimeMessage({
    topic: 'system.status',
    revision: loadMockDatabase().registryRevision,
    payload: {
      status: 'rebooting',
      rebooting: true,
    },
  })
  scheduleMockPersistenceFlush()
  return Promise.resolve(response)
}

function isRecordPayload(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeDashboardLayout(layout: DashboardLayoutRecord, deviceIds: number[]): DashboardLayoutRecord {
  const allowedIds = new Set(deviceIds)
  const panels = layout.panels.slice(0, 8).map((panel, index) => ({
    id: panel.id || `panel-${index + 1}`,
    name: panel.name.slice(0, 32),
    order: index,
    widgets: panel.widgets
      .filter(widget => allowedIds.has(widget.device_id))
      .map(widget => ({
        device_id: widget.device_id,
        x: Math.max(0, widget.x),
        y: Math.max(0, widget.y),
        w: Math.max(1, widget.w),
        h: Math.max(1, widget.h),
      })),
  }))
  if (panels.length === 0) {
    panels.push({
      id: 'main',
      name: 'Main panel',
      order: 0,
      widgets: deviceIds.map((deviceId, index) => ({
        device_id: deviceId,
        x: index % 6,
        y: Math.floor(index / 6),
        w: 1,
        h: 1,
      })),
    })
  }
  return {
    schema_version: 1,
    active_panel_id: panels.some(panel => panel.id === layout.active_panel_id) ? layout.active_panel_id : panels[0].id,
    panels,
  }
}

function pruneDashboardLayout(db: ReturnType<typeof createSeedMockDatabase>): void {
  db.dashboardLayout = normalizeDashboardLayout(db.dashboardLayout, db.devices.map(device => device.device_id))
}

function validateDashboardLayout(layout: DashboardLayoutRecord): void {
  if (layout.schema_version !== 1) {
    throw new ApiClientError('unsupported dashboard layout schema', 'UNSUPPORTED_SCHEMA', 400, null)
  }
  if (layout.panels.length === 0) {
    throw new ApiClientError('dashboard layout must contain at least one panel', 'EMPTY_PANELS', 400, null)
  }
  if (layout.panels.length > 8) {
    throw new ApiClientError('dashboard layout exceeds panel limit', 'TOO_MANY_PANELS', 400, null)
  }

  const ids = new Set<string>()
  const names = new Set<string>()
  for (const panel of layout.panels) {
    if (!panel.id || ids.has(panel.id)) {
      throw new ApiClientError('dashboard panel id is duplicated', 'DUPLICATE_PANEL_ID', 400, null)
    }
    if (!panel.name || panel.name.length > 32) {
      throw new ApiClientError('dashboard panel name is invalid', 'PANEL_NAME_TOO_LONG', 400, null)
    }
    const name = panel.name.toLowerCase()
    if (names.has(name)) {
      throw new ApiClientError('dashboard panel name is duplicated', 'DUPLICATE_PANEL_NAME', 400, null)
    }
    ids.add(panel.id)
    names.add(name)

    const widgetDeviceIds = new Set<number>()
    for (const widget of panel.widgets) {
      if (
        widget.device_id <= 0 ||
        widget.x < 0 ||
        widget.y < 0 ||
        widget.w <= 0 ||
        widget.h <= 0 ||
        widgetDeviceIds.has(widget.device_id)
      ) {
        throw new ApiClientError('dashboard widget coordinates are invalid', 'INVALID_WIDGET', 400, null)
      }
      widgetDeviceIds.add(widget.device_id)
    }
  }

  if (!ids.has(layout.active_panel_id)) {
    throw new ApiClientError('dashboard active panel does not exist', 'INVALID_ACTIVE_PANEL', 400, null)
  }
}
