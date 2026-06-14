import type {
  DashboardLayoutRecord,
  DashboardLayoutWidgetRecord,
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
import { DUMMY_DEVICE_TYPE_ID, GPIO_SWITCH_DEVICE_TYPE_ID } from '@/models/device-types'
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

export function mockSaveDashboardLayout(layout: DashboardLayoutRecord): Promise<void> {
  mutateRegistry(db => {
    validateDashboardLayout(layout)
    db.dashboardLayout = normalizeDashboardLayout(layout, db.devices.map(device => device.device_id))
    db.dashboardLayoutRevision += 1
  })
  const revision = loadMockDatabase().dashboardLayoutRevision
  publishRealtimeMessage({
    topic: 'dashboard.layout',
    revision,
    payload: {
      revision,
    },
  })
  return Promise.resolve()
}

export function mockCreateDevice(payload: Record<string, unknown>): Promise<DeviceMutationResponse> {
  const response = mutateRegistry(db => {
    const nextId = Math.max(1, ...db.devices.map(device => device.device_id)) + 1
    const typeId = payload.type_id
    if (typeof typeId !== 'number' || (typeId !== DUMMY_DEVICE_TYPE_ID && typeId !== GPIO_SWITCH_DEVICE_TYPE_ID)) {
      throw new ApiClientError('unsupported device type', 'UNSUPPORTED_TYPE', 400, null)
    }
    const isGpioSwitch = typeId === GPIO_SWITCH_DEVICE_TYPE_ID

    const device: DeviceRecord = {
      device_id: nextId,
      type_id: typeId,
      label: isGpioSwitch ? 'GPIO switch' : 'Dummy device',
      type: isGpioSwitch ? 'gpio_switch' : 'dummy',
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
        ...(isGpioSwitch
          ? {
              enabled: true,
              restore_previous_state: false,
              startup_state: 'off',
              safe_state: 'disabled',
              inverted: false,
              gpio_pin: 2,
            }
          : {
              enabled: true,
              restore_previous_state: false,
              default_output: false,
              current_output: false,
              inverted: false,
            }),
      },
      output: isGpioSwitch
        ? {
            state: 'off',
          }
        : undefined,
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
  const createdDevice = response.device ?? db.devices.at(-1)
  const deviceSnapshot = createdDevice
    ? {
        ...createdDevice,
        registry_revision: db.registryRevision,
        pending_persistence: db.pendingPersistence,
      }
    : { device_id: 0, registry_revision: db.registryRevision, pending_persistence: db.pendingPersistence }
  publishRealtimeMessage({
    topic: 'device.upsert',
    revision: db.registryRevision,
    payload: deviceSnapshot,
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
          if (device.type_id === GPIO_SWITCH_DEVICE_TYPE_ID) {
            if (payload.payload === 'state=on' || payload.payload === 'state=off' || payload.payload === 'state=disabled') {
              device.output = {
                state: payload.payload.replace('state=', '') as 'on' | 'off' | 'disabled',
              }
              break
            }
            throw new ApiClientError('unsupported gpio switch output state', 'BAD_ARGS', 400, null)
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
  const deviceSnapshot = response.device
    ? {
        ...response.device,
        registry_revision: db.registryRevision,
        pending_persistence: db.pendingPersistence,
      }
    : { device_id: deviceId, registry_revision: db.registryRevision, pending_persistence: db.pendingPersistence }
  publishRealtimeMessage({
    topic: payload.command === 'delete' ? 'device.remove' : 'device.command_result',
    revision: db.registryRevision,
    payload: deviceSnapshot,
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
      .filter(widget => allowedIds.has(widget[0]))
      .map(widget => [
        widget[0],
        Math.max(0, widget[1]),
        Math.max(0, widget[2]),
        Math.max(1, widget[3]),
        Math.max(1, widget[4]),
      ] as DashboardLayoutWidgetRecord),
  }))
  if (panels.length === 0) {
    panels.push({
      id: 'main',
      name: 'Main panel',
      order: 0,
      widgets: deviceIds.map((deviceId, index) => [deviceId, index % 6, Math.floor(index / 6), 1, 1]),
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
        widget[0] <= 0 ||
        widget[1] < 0 ||
        widget[2] < 0 ||
        widget[3] <= 0 ||
        widget[4] <= 0 ||
        widgetDeviceIds.has(widget[0])
      ) {
        throw new ApiClientError('dashboard widget coordinates are invalid', 'INVALID_WIDGET', 400, null)
      }
      widgetDeviceIds.add(widget[0])
    }
  }

  if (!ids.has(layout.active_panel_id)) {
    throw new ApiClientError('dashboard active panel does not exist', 'INVALID_ACTIVE_PANEL', 400, null)
  }
}
