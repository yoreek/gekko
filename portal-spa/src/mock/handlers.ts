import type {
  DashboardLayoutRecord,
  DashboardLayoutWidgetRecord,
  DashboardLayoutResponse,
  DeviceCommandRequest,
  DeviceDetailResponse,
  DeviceMutationResponse,
  DeviceSetupTransferResponse,
  DeviceRecord,
  DeviceRegistryResponse,
  OtaStatusResponse,
  SystemRestartResponse,
  WifiScanResponse,
  WifiStatusResponse,
} from '@/api'
import type { TemperatureOutputSnapshot } from '@/api/contracts'
import { ApiClientError } from '@/api/http'
import { publishRealtimeMessage } from '@/realtime/bus'
import { scheduleMockPersistenceFlush } from '@/realtime/mockRuntime'
import {
  DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
  DUMMY_DEVICE_TYPE_ID,
  GPIO_SWITCH_DEVICE_TYPE_ID,
  ONEWIRE_BUS_DEVICE_TYPE_ID,
  THERMOSTAT_DEVICE_TYPE_ID,
  deviceTypeIdFromName,
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
  publishDeviceUpsertMessage(db, device, 'device_updated')
}

function publishDeviceUpsertMessage(
  db: ReturnType<typeof createSeedMockDatabase>,
  device: DeviceRecord,
  eventKind: 'device_created' | 'device_updated' | 'snapshot',
): void {
  publishRealtimeMessage({
    topic: 'device.upsert',
    revision: db.registryRevision,
    payload: {
      ...device,
      event_kind: eventKind,
      registry_revision: db.registryRevision,
      pending_persistence: db.pendingPersistence,
    },
  })
}

function publishDeviceRemoveMessage(
  db: ReturnType<typeof createSeedMockDatabase>,
  device: DeviceRecord | undefined,
  eventKind: 'device_deleted' | 'command_rejected' = 'device_deleted',
): void {
  publishRealtimeMessage({
    topic: 'device.remove',
    revision: db.registryRevision,
    payload: {
      device_id: device?.device_id ?? 0,
      event_kind: eventKind,
      registry_revision: db.registryRevision,
      pending_persistence: db.pendingPersistence,
      name: device?.name ?? '',
      type_id: device?.type_id ?? 0,
      type: device?.type ?? device?.label ?? '',
    },
  })
}

function publishDeviceSnapshot(db: ReturnType<typeof createSeedMockDatabase>, device: DeviceRecord): void {
  publishDeviceUpsertMessage(db, device, 'snapshot')
}

function publishDependentThermostats(db: ReturnType<typeof createSeedMockDatabase>, sourceDeviceId: number): void {
  for (const device of db.devices) {
    if (device.type_id !== THERMOSTAT_DEVICE_TYPE_ID) {
      continue
    }
    const deps = Array.isArray(device.deps) ? device.deps : []
    if (deps.some(dep => dep.device_id === sourceDeviceId)) {
      publishDeviceSnapshot(db, device)
    }
  }
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
    db.dashboardLayout = normalizeDashboardLayout(layout, db.devices.map(device => device.device_id ?? 0))
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
    const nextId = Math.max(1, ...db.devices.map(device => device.device_id ?? 0)) + 1
    const typeName = typeof payload.type === 'string' ? payload.type : ''
    const typeId = typeof payload.type_id === 'number' ? payload.type_id : typeName.length > 0 ? deviceTypeIdFromName(typeName) : 0
    if (
      typeof typeId !== 'number' ||
      (
        typeId !== DUMMY_DEVICE_TYPE_ID &&
        typeId !== GPIO_SWITCH_DEVICE_TYPE_ID &&
        typeId !== ONEWIRE_BUS_DEVICE_TYPE_ID &&
        typeId !== DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID &&
        typeId !== THERMOSTAT_DEVICE_TYPE_ID
      )
    ) {
      throw new ApiClientError('unsupported device type', 'UNSUPPORTED_TYPE', 400, null)
    }
    const isGpioSwitch = typeId === GPIO_SWITCH_DEVICE_TYPE_ID
    const isOneWireBus = typeId === ONEWIRE_BUS_DEVICE_TYPE_ID
    const isDs18b20 = typeId === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID
    const isThermostat = typeId === THERMOSTAT_DEVICE_TYPE_ID
    const isDummy = typeId === DUMMY_DEVICE_TYPE_ID
    const dependencyLinks = isDs18b20 ? normalizeDependencyLinks(payload.deps) : []
    const configSource = isRecordPayload(payload.config) ? payload.config : payload
    const thermostatDependencyLinks = isThermostat ? normalizeThermostatDependencyLinks(payload.deps, configSource) : []
    const dependencyDeviceId = isDs18b20 ? dependencyDeviceIdForRole(dependencyLinks, 'onewire_bus') : 0
    if (isDs18b20) {
      requireOneWireDependency(db, dependencyDeviceId)
    }
    const enabled = Boolean(payload.enabled ?? true)
    const ds18b20Config = isDs18b20 ? normalizeDs18b20ConfigPayload(configSource, enabled) : undefined
    const thermostatConfig = isThermostat ? normalizeThermostatConfigPayload(configSource, enabled, thermostatDependencyLinks) : undefined
    if (isDs18b20 && ds18b20Config !== undefined) {
      ensureUniqueDs18b20Address(db, dependencyDeviceId, String(ds18b20Config.address), nextId)
    }
    if (isThermostat && thermostatConfig !== undefined) {
      requireThermostatDependencies(
        db,
        normalizeDependencyDeviceId(thermostatConfig.temperature_sensor_device_id),
        normalizeDependencyDeviceId(thermostatConfig.switch_device_id),
      )
    }

    const device: DeviceRecord = {
      device_id: nextId,
      type_id: typeId,
      label: isGpioSwitch
        ? 'GPIO switch'
        : isOneWireBus
          ? 'OneWire bus'
          : isDs18b20
            ? 'DS18B20 temperature sensor'
            : isThermostat
              ? 'Thermostat'
              : 'Dummy device',
      type: isGpioSwitch
        ? 'gpio_switch'
        : isOneWireBus
          ? 'onewire_bus'
          : isDs18b20
            ? 'ds18b20_temperature_sensor'
            : isThermostat
              ? 'thermostat'
              : 'dummy',
      name: String(payload.name ?? 'New Device'),
      enabled,
      deps: isThermostat ? thermostatDependencyLinks : dependencyLinks,
      has_deps: isThermostat ? thermostatDependencyLinks.length > 0 : dependencyLinks.length > 0,
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
            : isThermostat && thermostatConfig !== undefined
              ? thermostatConfig
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
          : isThermostat && thermostatConfig !== undefined
            ? buildThermostatOutput(db, thermostatConfig, nextId)
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
    if (!isDs18b20 && !isDummy && !isThermostat && isRecordPayload(payload.config)) {
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
    payload: {
      ...deviceSnapshot,
      event_kind: 'device_created',
    },
  })
  scheduleMockPersistenceFlush()
  return Promise.resolve(response)
}

export function mockCommandDevice(deviceId: number, payload: DeviceCommandRequest): Promise<DeviceMutationResponse> {
  let removedDevice: DeviceRecord | undefined
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
          removedDevice = { ...device }
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
            const dependencyLinks = normalizeDependencyLinks(payload.deps)
            const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'onewire_bus')
            if (dependencyDeviceId <= 0) {
              throw new ApiClientError('ds18b20 dependency is required', 'BAD_ARGS', 400, null)
            }
            requireOneWireDependency(db, dependencyDeviceId)
            const config = normalizeDs18b20ConfigPayload(payload.config, device.enabled)
            ensureUniqueDs18b20Address(db, dependencyDeviceId, String(config.address), device.device_id ?? 0)
            device.config = config
            device.enabled = Boolean(config.enabled)
            device.deps = dependencyLinks
            device.has_deps = dependencyLinks.length > 0
          } else if (device.type_id === THERMOSTAT_DEVICE_TYPE_ID) {
            const dependencyLinks = normalizeThermostatDependencyLinks(payload.deps, payload.config)
            const config = normalizeThermostatConfigPayload(payload.config, device.enabled, dependencyLinks)
            requireThermostatDependencies(
              db,
              normalizeDependencyDeviceId(config.temperature_sensor_device_id),
              normalizeDependencyDeviceId(config.switch_device_id),
            )
            device.config = config
            device.enabled = Boolean(config.enabled)
            device.deps = dependencyLinks
            device.has_deps = dependencyLinks.length > 0
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
  if (payload.command === 'delete') {
    publishRealtimeMessage({
      topic: 'device.remove',
      revision: db.registryRevision,
      payload: {
        device_id: removedDevice?.device_id ?? deviceId,
        event_kind: 'device_deleted',
        registry_revision: db.registryRevision,
        pending_persistence: db.pendingPersistence,
        name: removedDevice?.name ?? '',
        type_id: removedDevice?.type_id ?? 0,
        type: removedDevice?.type ?? removedDevice?.label ?? '',
      },
    })
  } else {
    publishRealtimeMessage({
      topic: 'device.command_result',
      revision: db.registryRevision,
      payload: {
        ...deviceSnapshot,
        event_kind: 'command_accepted',
      },
    })
  }
  publishDependentThermostats(db, deviceId)
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

function normalizeDependencyDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isInteger(numeric) && numeric > 0 ? numeric : 0
}

function normalizeDependencyLinks(value: unknown): Array<{ role: string; device_id: number }> {
  if (Array.isArray(value)) {
    return value
      .filter(isRecordPayload)
      .map(item => ({
        role: typeof item.role === 'string' ? item.role.trim() : '',
        device_id: normalizeDependencyDeviceId(item.device_id),
      }))
      .filter(item => item.role.length > 0 && item.device_id > 0)
  }
  return []
}

function dependencyDeviceIdForRole(deps: Array<{ role: string; device_id: number }>, role: string): number {
  const dependency = deps.find(link => link.role === role)
  return dependency?.device_id ?? 0
}

function normalizeThermostatDependencyLinks(value: unknown, fallbackConfig: unknown = null): Array<{ role: string; device_id: number }> {
  if (Array.isArray(value)) {
    const links = value
      .filter(isRecordPayload)
      .map(item => ({
        role: typeof item.role === 'string' ? item.role.trim() : '',
        device_id: normalizeDependencyDeviceId(item.device_id),
      }))
      .filter(item => item.role.length > 0 && item.device_id > 0)
    if (links.length > 0) {
      return links.filter(item => item.role === 'temperature_sensor' || item.role === 'switch')
    }
  }

  if (isRecordPayload(fallbackConfig)) {
    const temperatureSensorId = normalizeDependencyDeviceId(fallbackConfig.temperature_sensor_device_id)
    const switchDeviceId = normalizeDependencyDeviceId(fallbackConfig.switch_device_id)
    const links: Array<{ role: string; device_id: number }> = []
    if (temperatureSensorId > 0) {
      links.push({ role: 'temperature_sensor', device_id: temperatureSensorId })
    }
    if (switchDeviceId > 0) {
      links.push({ role: 'switch', device_id: switchDeviceId })
    }
    return links
  }

  return []
}

function normalizeThermostatMode(value: unknown): 'off' | 'heat' | 'cool' {
  return value === 'heat' || value === 'cool' ? value : 'off'
}

function normalizeThermostatAlgorithm(value: unknown): 'hysteresis' {
  return 'hysteresis'
}

function normalizeThermostatConfigPayload(
  value: unknown,
  enabledFallback: boolean,
  deps: Array<{ role: string; device_id: number }>,
): Record<string, unknown> {
  const current = isRecordPayload(value) ? value : {}
  const temperatureSensorId = normalizeDependencyDeviceId(current.temperature_sensor_device_id ?? dependencyDeviceIdForRole(deps, 'temperature_sensor'))
  const switchDeviceId = normalizeDependencyDeviceId(current.switch_device_id ?? dependencyDeviceIdForRole(deps, 'switch'))
  return {
    enabled: typeof current.enabled === 'boolean' ? current.enabled : enabledFallback,
    mode: normalizeThermostatMode(current.mode),
    algorithm: normalizeThermostatAlgorithm(current.algorithm),
    target_milli_celsius: Math.round(normalizeFiniteNumber(current.target_milli_celsius, 25000)),
    min_safe_milli_celsius: Math.round(normalizeFiniteNumber(current.min_safe_milli_celsius, 0)),
    max_safe_milli_celsius: Math.round(normalizeFiniteNumber(current.max_safe_milli_celsius, 50000)),
    hysteresis_centi_celsius: Math.max(0, Math.round(normalizeFiniteNumber(current.hysteresis_centi_celsius, 50))),
    check_interval_ms: Math.max(250, Math.round(normalizeFiniteNumber(current.check_interval_ms, 1000))),
    sensor_timeout_ms: Math.max(250, Math.round(normalizeFiniteNumber(current.sensor_timeout_ms, 6000))),
    retry_after_error_ms: Math.max(250, Math.round(normalizeFiniteNumber(current.retry_after_error_ms, 30000))),
    min_switch_interval_ms: Math.max(0, Math.round(normalizeFiniteNumber(current.min_switch_interval_ms, 5000))),
    temperature_sensor_device_id: temperatureSensorId,
    switch_device_id: switchDeviceId,
  }
}

function requireThermostatDependencies(db: ReturnType<typeof createSeedMockDatabase>, sensorDeviceId: number, switchDeviceId: number): void {
  const sensor = db.devices.find(entry => entry.device_id === sensorDeviceId)
  if (!sensor || sensor.type_id !== DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID) {
    throw new ApiClientError('thermostat temperature sensor dependency is required', 'BAD_ARGS', 400, null)
  }
  const switchDevice = db.devices.find(entry => entry.device_id === switchDeviceId)
  if (!switchDevice || switchDevice.type_id !== GPIO_SWITCH_DEVICE_TYPE_ID) {
    throw new ApiClientError('thermostat switch dependency is required', 'BAD_ARGS', 400, null)
  }
}

function buildThermostatOutput(
  db: ReturnType<typeof createSeedMockDatabase>,
  config: Record<string, unknown>,
  currentDeviceId: number,
): DeviceRecord['output'] {
  const sensorDeviceId = normalizeDependencyDeviceId(config.temperature_sensor_device_id)
  const switchDeviceId = normalizeDependencyDeviceId(config.switch_device_id)
  const sensor = db.devices.find(entry => entry.device_id === sensorDeviceId)
  const switchDevice = db.devices.find(entry => entry.device_id === switchDeviceId)
  const temperature = (sensor?.output as
    | { temperature?: { measured_at_ms?: number; valid?: boolean; value?: number; unit?: string; unit_symbol?: string } }
    | undefined
  )?.temperature
  const measuredAtMs = temperature?.measured_at_ms ?? 0
  const validTemperature = Boolean(temperature?.valid)
  const currentTemperature = validTemperature ? Number(temperature?.value ?? 0) : 0
  const hysteresis = Math.max(0, normalizeFiniteNumber(config.hysteresis_centi_celsius, 50)) / 100
  const target = normalizeFiniteNumber(config.target_milli_celsius, 25000) / 1000
  const mode = normalizeThermostatMode(config.mode)
  let desiredSwitchState: 'off' | 'on' | 'disabled' = 'off'
  let controlStatus = 'ready'

  if (!Boolean(config.enabled)) {
    desiredSwitchState = 'disabled'
    controlStatus = 'disabled'
  } else if (!sensor || sensor.type_id !== DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID || !switchDevice || switchDevice.type_id !== GPIO_SWITCH_DEVICE_TYPE_ID) {
    desiredSwitchState = 'off'
    controlStatus = 'dependency_blocked'
  } else if (!sensor.enabled || sensor.effective_status !== 'ready' || !validTemperature) {
    desiredSwitchState = 'off'
    controlStatus = 'sensor_timeout'
  } else if (mode === 'off') {
    desiredSwitchState = 'off'
    controlStatus = 'idle'
  } else if (mode === 'heat') {
    if (currentTemperature <= target - hysteresis) {
      desiredSwitchState = 'on'
      controlStatus = 'heating'
    } else if (currentTemperature >= target + hysteresis) {
      desiredSwitchState = 'off'
      controlStatus = 'idle'
    } else {
      desiredSwitchState = ((switchDevice.output as { state?: 'off' | 'on' | 'disabled' } | undefined)?.state === 'on' ? 'on' : 'off')
      controlStatus = desiredSwitchState === 'on' ? 'heating' : 'idle'
    }
  } else {
    if (currentTemperature >= target + hysteresis) {
      desiredSwitchState = 'on'
      controlStatus = 'cooling'
    } else if (currentTemperature <= target - hysteresis) {
      desiredSwitchState = 'off'
      controlStatus = 'idle'
    } else {
      desiredSwitchState = ((switchDevice.output as { state?: 'off' | 'on' | 'disabled' } | undefined)?.state === 'on' ? 'on' : 'off')
      controlStatus = desiredSwitchState === 'on' ? 'cooling' : 'idle'
    }
  }

  const actualSwitchState = (switchDevice?.output as { state?: 'off' | 'on' | 'disabled' } | undefined)?.state ?? 'off'
  if (desiredSwitchState !== 'disabled' && actualSwitchState !== desiredSwitchState && controlStatus !== 'dependency_blocked' && controlStatus !== 'sensor_timeout') {
    controlStatus = 'switch_error'
  }

  const outputTemperature: TemperatureOutputSnapshot = (temperature as TemperatureOutputSnapshot | undefined) ?? {
    value: 0,
    unit: 'celsius',
    unit_symbol: 'C',
    measured_at_ms: 0,
    valid: false,
    status: 'not_ready',
  }

  return {
    ...((db.devices.find(entry => entry.device_id === currentDeviceId)?.output ?? {}) as Record<string, unknown>),
    temperature: outputTemperature,
    desired_switch_state: desiredSwitchState,
    actual_switch_state: actualSwitchState,
    control_status: controlStatus,
    last_check_at_ms: measuredAtMs,
  }
}

function requireOneWireDependency(db: ReturnType<typeof createSeedMockDatabase>, dependencyDeviceId: number): DeviceRecord {
  const dependency = db.devices.find(entry => entry.device_id === dependencyDeviceId)
  if (!dependency || dependency.type_id !== ONEWIRE_BUS_DEVICE_TYPE_ID) {
    throw new ApiClientError('valid onewire dependency is required', 'BAD_ARGS', 400, null)
  }
  return dependency
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
  dependencyDeviceId: number,
  address: string,
  currentDeviceId: number,
): void {
  const normalizedAddress = address.trim().toUpperCase()
  const duplicate = db.devices.some(device => (
    device.device_id !== currentDeviceId &&
    device.type_id === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID &&
    dependencyDeviceIdForRole(device.deps ?? [], 'onewire_bus') === dependencyDeviceId &&
    isRecordPayload(device.config) &&
    typeof device.config.address === 'string' &&
    device.config.address.trim().toUpperCase() === normalizedAddress
  ))
  if (duplicate) {
    throw new ApiClientError('ds18b20 address already exists on this dependency', 'DUPLICATE_ADDRESS', 400, null)
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
    const dependencyDeviceId = dependencyDeviceIdForRole(device.deps ?? [], 'onewire_bus')
    const dependency = db.devices.find(entry => entry.device_id === dependencyDeviceId)
    if (!dependency || dependency.type_id !== ONEWIRE_BUS_DEVICE_TYPE_ID) {
      device.effective_status = 'dependency_blocked'
      device.status = 'dependency_blocked'
      markTemperatureUnavailable(device, 'missing_dependency')
      continue
    }
    if (!dependency.enabled || dependency.effective_status === 'disabled') {
      device.effective_status = 'disabled'
      device.status = 'disabled'
      markTemperatureUnavailable(device, 'dependency_disabled')
      continue
    }
    if (dependency.effective_status !== 'ready') {
      device.effective_status = 'dependency_blocked'
      device.status = 'dependency_blocked'
      markTemperatureUnavailable(device, 'dependency_not_ready')
      continue
    }

    device.effective_status = device.lifecycle_status === 'faulted' ? 'faulted' : 'ready'
    device.status = device.effective_status
    if (!(device.output as { temperature?: unknown } | undefined)?.temperature) {
      markTemperatureUnavailable(device, 'not_ready')
    }
  }

  for (const device of db.devices) {
    if (device.type_id !== THERMOSTAT_DEVICE_TYPE_ID) {
      continue
    }
    const config = isRecordPayload(device.config) ? device.config : {}
    const sensor = db.devices.find(entry => entry.device_id === normalizeDependencyDeviceId(config.temperature_sensor_device_id))
    const switchDevice = db.devices.find(entry => entry.device_id === normalizeDependencyDeviceId(config.switch_device_id))

    if (!device.enabled) {
      device.lifecycle_status = 'disabled'
      device.effective_status = 'disabled'
      device.status = 'disabled'
      device.output = buildThermostatOutput(db, config, device.device_id ?? 0)
      continue
    }

    if (device.lifecycle_status === 'disabled') {
      device.lifecycle_status = 'ready'
    }
    if (!sensor || sensor.type_id !== DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID || !switchDevice || switchDevice.type_id !== GPIO_SWITCH_DEVICE_TYPE_ID) {
      device.effective_status = 'dependency_blocked'
      device.status = 'dependency_blocked'
      device.output = buildThermostatOutput(db, config, device.device_id ?? 0)
      continue
    }
    if (!sensor.enabled || sensor.effective_status === 'disabled') {
      device.effective_status = 'disabled'
      device.status = 'disabled'
      device.output = buildThermostatOutput(db, config, device.device_id ?? 0)
      continue
    }

    device.effective_status = device.lifecycle_status === 'faulted' ? 'faulted' : 'ready'
    device.status = device.effective_status
    device.output = buildThermostatOutput(db, config, device.device_id ?? 0)
  }
}

export function refreshMockDerivedDeviceState(db: ReturnType<typeof createSeedMockDatabase>): void {
  refreshChildEffectiveStatuses(db)
}

export function publishThermostatDependents(db: ReturnType<typeof createSeedMockDatabase>, sourceDeviceId: number): void {
  publishDependentThermostats(db, sourceDeviceId)
}

function cloneConfig(value: unknown): Record<string, unknown> {
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return {}
  }
  return { ...(value as Record<string, unknown>) }
}

function dependencyIdFromExportDeps(deps: unknown, role: string): number {
  if (!Array.isArray(deps)) {
    return 0
  }
  const match = deps.find(entry => typeof entry === 'object' && entry !== null && !Array.isArray(entry) && (entry as Record<string, unknown>).role === role)
  if (match === undefined) {
    return 0
  }
  const deviceId = Number((match as Record<string, unknown>).device_id ?? 0)
  return Number.isFinite(deviceId) && deviceId > 0 ? deviceId : 0
}

function exportDeviceConfig(device: DeviceRecord): Record<string, unknown> {
  const config = cloneConfig(device.config)
  if (device.type_id === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID) {
    delete config.dependency_device_id
  }
  if (device.type_id === THERMOSTAT_DEVICE_TYPE_ID) {
    delete config.temperature_sensor_device_id
    delete config.switch_device_id
  }
  return config
}

export function mockExportDeviceSetupBundle(): string {
  const db = loadMockDatabase()
  const lines: string[] = []
  lines.push(JSON.stringify({
    kind: 'transfer_envelope',
    transfer_schema_version: 1,
    registry_schema_version: 1,
    registry_revision: db.registryRevision,
    device_count: db.devices.length,
  }))

  for (const device of db.devices) {
    const config = exportDeviceConfig(device)
    lines.push(JSON.stringify({
      kind: 'device',
      id: device.device_id,
      type: device.type,
      config_version: device.config_version,
      config_revision: device.config_revision,
      ...config,
      name: device.name,
      enabled: device.enabled,
      deps: Array.isArray(device.deps) ? device.deps : [],
    }))
  }

  return `${lines.join('\n')}\n`
}

export function mockImportDeviceSetupBundle(file: File): Promise<DeviceSetupTransferResponse> {
  return file.text().then(text => {
    const lines = text.split(/\r?\n/).map(line => line.trim()).filter(Boolean)
    if (lines.length === 0) {
      throw new ApiClientError('bundle file is missing', 'BAD_JSON', 400, null)
    }

    const envelope = JSON.parse(lines[0]) as Record<string, unknown>
    if (envelope.kind !== 'transfer_envelope' || envelope.transfer_schema_version !== 1) {
      throw new ApiClientError('unsupported transfer schema version', 'INVALID_VERSION', 400, null)
    }

    const devices: DeviceRecord[] = []
    for (const line of lines.slice(1)) {
      const parsed = JSON.parse(line) as Record<string, unknown>
      if (parsed.kind !== 'device') {
        throw new ApiClientError('unexpected bundle record kind', 'BAD_ARGS', 400, null)
      }
      const config = cloneConfig(parsed)
      delete config.kind
      delete config.id
      delete config.device_id
      delete config.type
      delete config.type_id
      delete config.config_version
      delete config.config_revision
      delete config.name
      delete config.enabled
      delete config.deps
      delete config.config_blob_hex

      const typeName = typeof parsed.type === 'string' ? parsed.type : ''
      const deps = (Array.isArray(parsed.deps) ? parsed.deps : []) as NonNullable<DeviceRecord['deps']>
      if (typeName === 'ds18b20_temperature_sensor') {
        const dependencyDeviceId = dependencyIdFromExportDeps(deps, 'onewire_bus')
        if (dependencyDeviceId > 0) {
          config.dependency_device_id = dependencyDeviceId
        }
      }
      if (typeName === 'thermostat') {
        const temperatureSensorId = dependencyIdFromExportDeps(deps, 'temperature_sensor')
        const switchDeviceId = dependencyIdFromExportDeps(deps, 'switch')
        if (temperatureSensorId > 0) {
          config.temperature_sensor_device_id = temperatureSensorId
        }
        if (switchDeviceId > 0) {
          config.switch_device_id = switchDeviceId
        }
      }

      devices.push({
        device_id: Number(parsed.id ?? parsed.device_id ?? 0),
        type_id: Number(parsed.type_id ?? deviceTypeIdFromName(typeName)),
        label: typeof parsed.name === 'string' ? parsed.name : undefined,
        type: typeName,
        name: typeof parsed.name === 'string' ? parsed.name : '',
        enabled: Boolean(parsed.enabled),
        deps,
        has_deps: deps.length > 0,
        config_version: Number(parsed.config_version ?? 0),
        config_revision: Number(parsed.config_revision ?? 0),
        lifecycle_status: 'ready',
        effective_status: 'ready',
        status: 'ready',
        config,
      })
    }

    mutateRegistry(db => {
      db.devices = devices
      db.registryRevision = Number(envelope.registry_revision ?? db.registryRevision)
      db.pendingPersistence = false
      refreshMockDerivedDeviceState(db)
      return null
    })

    return ok({
      registry_revision: Number(envelope.registry_revision ?? 0),
      device_count: devices.length,
    })
  })
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
  db.dashboardLayout = normalizeDashboardLayout(db.dashboardLayout, db.devices.map(device => device.device_id ?? 0))
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
