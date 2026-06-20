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
import {
  DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
  DUMMY_DEVICE_TYPE_ID,
  GPIO_SWITCH_DEVICE_TYPE_ID,
  ONEWIRE_BUS_DEVICE_TYPE_ID,
} from '@/models/device-types'
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

function completeOneWireScan(deviceId: number): void {
  const db = loadMockDatabase()
  const device = db.devices.find(entry => entry.device_id === deviceId)
  if (!device || device.type_id !== ONEWIRE_BUS_DEVICE_TYPE_ID) {
    return
  }
  device.scan = {
    in_progress: false,
    ready: true,
    device_count: 2,
    truncated: false,
    invalid_crc_seen: false,
    devices: [
      {
        address: '28FF641D621603AD',
        family_code: '28',
      },
      {
        address: '10FFAA0000000001',
        family_code: '10',
      },
    ],
  }
  saveMockDatabase(db)
  publishRealtimeMessage({
    topic: 'device.upsert',
    revision: db.registryRevision,
    payload: {
      ...device,
      registry_revision: db.registryRevision,
      pending_persistence: db.pendingPersistence,
    },
  })
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

export function mockResetWifiCredentials(): Promise<{ status: string; action: string }> {
  const response = mutateRegistry(db => {
    db.wifi.status = 'idle'
    db.wifi.stationIp = ''
    db.registryRevision += 1
    db.pendingPersistence = true
    return ok({
      status: 'accepted',
      action: 'clear_wifi_credentials',
    })
  })
  publishRealtimeMessage({
    topic: 'wifi.status',
    revision: loadMockDatabase().registryRevision,
    payload: {
      wifi_status: 'idle',
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
    if (
      typeof typeId !== 'number' ||
      (
        typeId !== DUMMY_DEVICE_TYPE_ID &&
        typeId !== GPIO_SWITCH_DEVICE_TYPE_ID &&
        typeId !== ONEWIRE_BUS_DEVICE_TYPE_ID &&
        typeId !== DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID
      )
    ) {
      throw new ApiClientError('unsupported device type', 'UNSUPPORTED_TYPE', 400, null)
    }
    const isGpioSwitch = typeId === GPIO_SWITCH_DEVICE_TYPE_ID
    const isOneWireBus = typeId === ONEWIRE_BUS_DEVICE_TYPE_ID
    const isDs18b20 = typeId === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID
    const isDummy = typeId === DUMMY_DEVICE_TYPE_ID
    const parentDeviceId = isDs18b20 ? normalizeParentDeviceId(payload.parent_device_id) : 0
    if (isDs18b20) {
      requireOneWireParent(db, parentDeviceId)
    }
    const enabled = Boolean(payload.enabled ?? true)
    const ds18b20Config = isDs18b20 ? normalizeDs18b20ConfigPayload(payload.config, enabled) : undefined
    if (isDs18b20 && ds18b20Config !== undefined) {
      ensureUniqueDs18b20Address(db, parentDeviceId, String(ds18b20Config.address), nextId)
    }

    const device: DeviceRecord = {
      device_id: nextId,
      type_id: typeId,
      label: isGpioSwitch ? 'GPIO switch' : isOneWireBus ? 'OneWire bus' : isDs18b20 ? 'DS18B20 temperature sensor' : 'Dummy device',
      type: isGpioSwitch ? 'gpio_switch' : isOneWireBus ? 'onewire_bus' : isDs18b20 ? 'ds18b20_temperature_sensor' : 'dummy',
      name: String(payload.name ?? 'New Device'),
      enabled,
      has_parent: isDs18b20,
      parent_device_id: parentDeviceId,
      config_version: 1,
      config_revision: 1,
      lifecycle_status: 'ready',
      effective_status: 'ready',
      status: 'ready',
      retained_state_supported: isGpioSwitch,
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
          : isOneWireBus
            ? {
                enabled: true,
                gpio_pin: 4,
                internal_pullup: false,
              }
          : isDs18b20 && ds18b20Config !== undefined
            ? ds18b20Config
          : isDummy
            ? {
                enabled,
                name: String(payload.name ?? 'New Device'),
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
        : isDs18b20
          ? {
              temperature: {
                value: 0,
                unit: ds18b20Config?.unit === 'fahrenheit' ? 'fahrenheit' : 'celsius',
                unit_symbol: ds18b20Config?.unit === 'fahrenheit' ? 'F' : 'C',
                measured_at_ms: 0,
                valid: false,
                status: 'not_ready',
              },
            }
        : undefined,
      scan: isOneWireBus
        ? {
            in_progress: false,
            ready: false,
            device_count: 0,
            truncated: false,
            invalid_crc_seen: false,
            devices: [],
          }
        : undefined,
    }
    if (!isDs18b20 && !isDummy && isRecordPayload(payload.config)) {
      device.config = {
        ...device.config,
        ...payload.config,
      }
    }
    db.devices.push(device)
    refreshChildEffectiveStatuses(db)
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
          if (typeof payload.name !== 'string' || payload.name.trim().length === 0) {
            throw new ApiClientError('name is required', 'BAD_ARGS', 400, null)
          }
          device.name = payload.name
          break
        case 'enable':
          device.enabled = true
          device.lifecycle_status = 'ready'
          device.effective_status = 'ready'
          device.status = 'ready'
          if (isRecordPayload(device.config)) {
            device.config.enabled = true
          }
          break
        case 'disable':
          device.enabled = false
          device.lifecycle_status = 'disabled'
          device.effective_status = 'disabled'
          device.status = 'disabled'
          if (isRecordPayload(device.config)) {
            device.config.enabled = false
          }
          break
        case 'delete':
          db.devices = db.devices.filter(entry => entry.device_id !== deviceId)
          pruneDashboardLayout(db)
          break
        case 'update_config':
          if (!isRecordPayload(payload.config)) {
            throw new ApiClientError('config is required', 'BAD_ARGS', 400, null)
          }
          if (device.type_id === GPIO_SWITCH_DEVICE_TYPE_ID) {
            const currentConfig = isRecordPayload(device.config) ? device.config : {}
            device.config = {
              enabled: Boolean(payload.config.enabled ?? device.enabled),
              restore_previous_state: Boolean(payload.config.restore_previous_state ?? false),
              startup_state: payload.config.startup_state === 'on'
                ? 'on'
                : payload.config.startup_state === 'disabled'
                  ? 'disabled'
                  : 'off',
              safe_state: payload.config.safe_state === 'on'
                ? 'on'
                : payload.config.safe_state === 'disabled'
                  ? 'disabled'
                  : 'off',
              inverted: Boolean(payload.config.inverted ?? false),
              gpio_pin: normalizeFiniteNumber(payload.config.gpio_pin, normalizeFiniteNumber(currentConfig.gpio_pin, 2)),
            }
            device.enabled = Boolean(device.config.enabled)
          } else if (device.type_id === ONEWIRE_BUS_DEVICE_TYPE_ID) {
            const currentConfig = isRecordPayload(device.config) ? device.config : {}
            device.config = {
              enabled: Boolean(payload.config.enabled ?? device.enabled),
              gpio_pin: normalizeFiniteNumber(payload.config.gpio_pin, normalizeFiniteNumber(currentConfig.gpio_pin, 4)),
              internal_pullup: Boolean(payload.config.internal_pullup ?? false),
            }
            device.enabled = Boolean(device.config.enabled)
          } else if (device.type_id === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID) {
            const parentDeviceId = normalizeParentDeviceId(payload.parent_device_id)
            if (payload.has_parent !== true || parentDeviceId <= 0) {
              throw new ApiClientError('ds18b20 parent is required', 'BAD_ARGS', 400, null)
            }
            requireOneWireParent(db, parentDeviceId)
            const config = normalizeDs18b20ConfigPayload(payload.config, device.enabled)
            ensureUniqueDs18b20Address(db, parentDeviceId, String(config.address), device.device_id)
            device.config = config
            device.enabled = Boolean(config.enabled)
            device.has_parent = true
            device.parent_device_id = parentDeviceId
          }
          device.config_revision += 1
          break
        case 'set_status':
          if (device.type_id === DUMMY_DEVICE_TYPE_ID) {
            throw new ApiClientError('unsupported dummy command', 'BAD_ARGS', 400, null)
          }
          if (payload.status === 'fault') {
            device.lifecycle_status = 'faulted'
            device.effective_status = 'faulted'
            device.status = 'faulted'
          }
          if (payload.status === 'ready') {
            device.lifecycle_status = 'ready'
            device.effective_status = 'ready'
            device.status = 'ready'
          }
          break
        case 'scan':
          if (device.type_id !== ONEWIRE_BUS_DEVICE_TYPE_ID) {
            throw new ApiClientError('unsupported onewire command', 'BAD_ARGS', 400, null)
          }
          if (!isRecordPayload(device.config)) {
            device.config = {}
          }
          if (isRecordPayload(device.scan) && device.scan.in_progress === true) {
            throw new ApiClientError('scan already in progress', 'BAD_ARGS', 400, null)
          }
          device.scan = {
            in_progress: true,
            ready: false,
            device_count: 0,
            truncated: false,
            invalid_crc_seen: false,
            devices: [],
          }
          return ok({
            registry_revision: db.registryRevision,
            pending_persistence: db.pendingPersistence,
            device,
          })
        case 'set_output':
          if (typeof payload.state !== 'string') {
            throw new ApiClientError('state is required', 'BAD_ARGS', 400, null)
          }
          if (device.type_id === GPIO_SWITCH_DEVICE_TYPE_ID) {
          if (payload.state !== 'on' && payload.state !== 'off' && payload.state !== 'disabled') {
              throw new ApiClientError('unsupported gpio switch output state', 'BAD_ARGS', 400, null)
            }
            device.output = {
              state: payload.state,
            }
          } else {
            throw new ApiClientError('unsupported output command', 'BAD_ARGS', 400, null)
          }
          break
        case 'set_parent':
          if (payload.has_parent === undefined || payload.parent_device_id === undefined) {
            throw new ApiClientError('has_parent and parent_device_id are required', 'BAD_ARGS', 400, null)
          }
          if (device.type_id === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID) {
            const parentDeviceId = normalizeParentDeviceId(payload.parent_device_id)
            if (payload.has_parent !== true || parentDeviceId <= 0) {
              throw new ApiClientError('ds18b20 parent is required', 'BAD_ARGS', 400, null)
            }
            requireOneWireParent(db, parentDeviceId)
            if (isRecordPayload(device.config) && typeof device.config.address === 'string') {
              ensureUniqueDs18b20Address(db, parentDeviceId, device.config.address, device.device_id)
            }
            device.has_parent = true
            device.parent_device_id = parentDeviceId
          } else {
            device.has_parent = Boolean(payload.has_parent)
            device.parent_device_id = Number(payload.parent_device_id)
          }
          break
        default:
          break
      }

      refreshChildEffectiveStatuses(db)
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
  if (
    payload.command === 'scan' &&
    response.device?.type_id === ONEWIRE_BUS_DEVICE_TYPE_ID &&
    typeof window !== 'undefined'
  ) {
    window.setTimeout(() => {
      completeOneWireScan(deviceId)
    }, 100)
  }
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

function normalizeParentDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isInteger(numeric) && numeric > 0 ? numeric : 0
}

function requireOneWireParent(db: ReturnType<typeof createSeedMockDatabase>, parentDeviceId: number): DeviceRecord {
  const parent = db.devices.find(entry => entry.device_id === parentDeviceId)
  if (!parent || parent.type_id !== ONEWIRE_BUS_DEVICE_TYPE_ID) {
    throw new ApiClientError('valid onewire parent is required', 'BAD_ARGS', 400, null)
  }
  return parent
}

function ds18b20AddressShapeValid(address: string): boolean {
  return /^[0-9A-Fa-f]{16}$/.test(address.trim())
}

function normalizeDs18b20Resolution(value: unknown): number {
  return value === 9 || value === 10 || value === 11 || value === 12 ? value : 12
}

function normalizeDs18b20Unit(value: unknown): 'celsius' | 'fahrenheit' {
  return value === 'fahrenheit' ? 'fahrenheit' : 'celsius'
}

function normalizeFiniteNumber(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) ? numeric : fallback
}

function normalizeDs18b20ConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid ds18b20 config', 'BAD_ARGS', 400, null)
  }
  const address = typeof value.address === 'string' ? value.address.trim().toUpperCase() : ''
  if (!ds18b20AddressShapeValid(address)) {
    throw new ApiClientError('invalid ds18b20 address', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    address,
    resolution: normalizeDs18b20Resolution(value.resolution),
    unit: normalizeDs18b20Unit(value.unit),
    poll_ms: Math.max(1000, normalizeFiniteNumber(value.poll_ms, 5000)),
    report_delta_celsius: Math.max(0.01, normalizeFiniteNumber(value.report_delta_celsius, 0.01)),
    report_always: typeof value.report_always === 'boolean' ? value.report_always : false,
  }
}

function ensureUniqueDs18b20Address(
  db: ReturnType<typeof createSeedMockDatabase>,
  parentDeviceId: number,
  address: string,
  currentDeviceId: number,
): void {
  const normalizedAddress = address.trim().toUpperCase()
  const duplicate = db.devices.some(device => (
    device.device_id !== currentDeviceId &&
    device.type_id === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID &&
    device.parent_device_id === parentDeviceId &&
    isRecordPayload(device.config) &&
    typeof device.config.address === 'string' &&
    device.config.address.trim().toUpperCase() === normalizedAddress
  ))
  if (duplicate) {
    throw new ApiClientError('ds18b20 address already exists on this parent', 'DUPLICATE_ADDRESS', 400, null)
  }
}

function markTemperatureUnavailable(device: DeviceRecord, status: string): void {
  const unit = isRecordPayload(device.config) && device.config.unit === 'fahrenheit' ? 'fahrenheit' : 'celsius'
  device.output = {
    ...device.output,
    temperature: {
      value: 0,
      unit,
      unit_symbol: unit === 'fahrenheit' ? 'F' : 'C',
      measured_at_ms: 0,
      valid: false,
      status,
    },
  }
}

function refreshChildEffectiveStatuses(db: ReturnType<typeof createSeedMockDatabase>): void {
  for (const device of db.devices) {
    if (device.type_id !== DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID) {
      continue
    }
    if (!device.enabled) {
      device.lifecycle_status = 'disabled'
      device.effective_status = 'disabled'
      device.status = 'disabled'
      markTemperatureUnavailable(device, 'disabled')
      continue
    }

    if (device.lifecycle_status === 'disabled') {
      device.lifecycle_status = 'ready'
    }
    const parent = db.devices.find(entry => entry.device_id === device.parent_device_id)
    if (!parent || parent.type_id !== ONEWIRE_BUS_DEVICE_TYPE_ID) {
      device.effective_status = 'dependency_blocked'
      device.status = 'dependency_blocked'
      markTemperatureUnavailable(device, 'missing_parent')
      continue
    }
    if (!parent.enabled || parent.effective_status === 'disabled') {
      device.effective_status = 'disabled'
      device.status = 'disabled'
      markTemperatureUnavailable(device, 'parent_disabled')
      continue
    }
    if (parent.effective_status !== 'ready') {
      device.effective_status = 'dependency_blocked'
      device.status = 'dependency_blocked'
      markTemperatureUnavailable(device, 'parent_not_ready')
      continue
    }

    device.effective_status = device.lifecycle_status === 'faulted' ? 'faulted' : 'ready'
    device.status = device.effective_status
    if (!device.output?.temperature) {
      markTemperatureUnavailable(device, 'not_ready')
    }
  }
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
