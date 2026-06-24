import type {
  DashboardLayoutRecord,
  DashboardLayoutWidgetRecord,
  DashboardLayoutResponse,
  DeviceCommandRequest,
  DeviceDetailResponse,
  DeviceCreateRequest,
  DeviceMutationResponse,
  DeviceSetupTransferResponse,
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

type DeviceRecord = Record<string, any>

function ok<T extends object>(payload: T): T & { success: true } {
  return {
    success: true,
    ...payload,
  }
}

export function decorateDeviceRecord(
  device: DeviceRecord,
  registryRevision: number,
): DeviceRecord {
  const typeName = typeof device.record?.typeName === 'string' && device.record.typeName.trim().length > 0
    ? device.record.typeName.trim()
    : typeof device.typeName === 'string' && device.typeName.trim().length > 0
      ? device.typeName.trim()
      : ''
  const configRevision = typeof device.record?.configRevision === 'number'
    ? device.record.configRevision
    : Number(device.configRevision ?? 0)
  const id = Number(device.deviceId ?? 0)
  const config = isRecordPayload(device.config) ? { ...device.config } : {}
  const deps = Array.isArray(device.deps) ? device.deps : []
  const runtime = {
    status: device.status ?? device.effectiveStatus ?? device.lifecycleStatus ?? 'unknown',
    lifecycleStatus: device.lifecycleStatus ?? device.status ?? 'unknown',
    effectiveStatus: device.effectiveStatus ?? device.status ?? device.lifecycleStatus ?? 'unknown',
  } as Record<string, unknown>

  if (device.output !== undefined) {
    runtime.output = device.output
  }
  if (device.scan !== undefined) {
    runtime.scan = device.scan
  }

  return {
    record: {
      id,
      typeName,
      configRevision,
    },
    config: {
      name: device.name,
      enabled: device.enabled,
      deps,
      ...config,
    },
    runtime,
    registryRevision,
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
  const device = db.devices.find(entry => entry.deviceId === deviceId)
  if (!device || device.typeId !== ONEWIRE_BUS_DEVICE_TYPE_ID) {
    return
  }
  device.scan = {
    inProgress: false,
    ready: true,
    deviceCount: 2,
    truncated: false,
    invalidCrcSeen: false,
    devices: [
      {
        address: '28FF641D621603AD',
        familyCode: '28',
      },
      {
        address: '10FFAA0000000001',
        familyCode: '10',
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
  const payload = decorateDeviceRecord(device, db.registryRevision)
  publishRealtimeMessage({
    topic: 'device.upsert',
    revision: db.registryRevision,
    payload: {
      ...payload,
      eventKind,
    },
  })
}

function publishDeviceRemoveMessage(
  db: ReturnType<typeof createSeedMockDatabase>,
  device: DeviceRecord | undefined,
  eventKind: 'device_deleted' | 'command_rejected' = 'device_deleted',
): void {
  const payload = device !== undefined ? decorateDeviceRecord(device, db.registryRevision) : undefined
  publishRealtimeMessage({
    topic: 'device.remove',
    revision: db.registryRevision,
    payload: {
      ...payload,
      deviceId: payload?.record?.id ?? 0,
      eventKind,
    },
  })
}

function publishDeviceSnapshot(db: ReturnType<typeof createSeedMockDatabase>, device: DeviceRecord): void {
  publishDeviceUpsertMessage(db, device, 'snapshot')
}

function publishDependentThermostats(db: ReturnType<typeof createSeedMockDatabase>, sourceDeviceId: number): void {
  for (const device of db.devices) {
    if (device.typeId !== THERMOSTAT_DEVICE_TYPE_ID) {
      continue
    }
    const deps = Array.isArray(device.deps) ? device.deps : []
    if (deps.some(dep => dep.deviceId === sourceDeviceId)) {
      publishDeviceSnapshot(db, device)
    }
  }
}

export function mockFetchWifiStatus(): WifiStatusResponse {
  const db = loadMockDatabase()
  return ok({
    wifiStatus: db.wifi.status,
    stationIp: db.wifi.stationIp,
    setupApIp: db.wifi.setupApIp,
  }) as unknown as WifiStatusResponse
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
      wifiStatus: db.wifi.status,
      stationIp: db.wifi.stationIp,
      setupApIp: db.wifi.setupApIp,
    },
  })
  scheduleMockPersistenceFlush()
  return Promise.resolve(response)
}

export function mockStartBleWifiConfig(): Promise<{ status: string; action: string }> {
  const response = mutateRegistry(db => {
    db.wifi.status = 'ble_config'
    db.registryRevision += 1
    return ok({
      status: 'accepted',
      action: 'start_ble_config',
    })
  })
  publishRealtimeMessage({
    topic: 'wifi.status',
    revision: loadMockDatabase().registryRevision,
    payload: {
      wifiStatus: 'ble_config',
      stationIp: loadMockDatabase().wifi.stationIp,
      setupApIp: loadMockDatabase().wifi.setupApIp,
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
    return ok({
      status: 'accepted',
      action: 'clear_wifi_credentials',
    })
  })
  publishRealtimeMessage({
    topic: 'wifi.status',
    revision: loadMockDatabase().registryRevision,
    payload: {
      wifiStatus: 'idle',
      stationIp: loadMockDatabase().wifi.stationIp,
      setupApIp: loadMockDatabase().wifi.setupApIp,
    },
  })
  scheduleMockPersistenceFlush()
  return Promise.resolve(response)
}

export function mockFetchDevices(): DeviceRegistryResponse {
  const db = loadMockDatabase()
  return ok({
    registryRevision: db.registryRevision,
    devices: db.devices.map(device => decorateDeviceRecord(device, db.registryRevision)),
  }) as unknown as DeviceRegistryResponse
}

export function mockFetchDevice(deviceId: number): DeviceDetailResponse {
  const db = loadMockDatabase()
  const device = db.devices.find(entry => entry.deviceId === deviceId)
  if (!device) {
    throw new ApiClientError('device not found', 'NOT_FOUND', 404, null)
  }
  return ok({
    registryRevision: db.registryRevision,
    device: decorateDeviceRecord(device, db.registryRevision),
  }) as unknown as DeviceDetailResponse
}

export function mockFetchDashboardLayout(): DashboardLayoutResponse {
  const db = loadMockDatabase()
  pruneDashboardLayout(db)
  saveMockDatabase(db)
  return ok({
    revision: db.dashboardLayoutRevision,
    layoutDefaulted: false,
    layout: db.dashboardLayout,
  })
}

export function mockSaveDashboardLayout(layout: DashboardLayoutRecord): Promise<void> {
  mutateRegistry(db => {
    validateDashboardLayout(layout)
    db.dashboardLayout = normalizeDashboardLayout(layout, db.devices.map(device => device.deviceId ?? 0))
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

export function mockCreateDevice(payload: DeviceCreateRequest | Record<string, unknown>): Promise<DeviceMutationResponse> {
  const response = mutateRegistry(db => {
    const nextId = Math.max(1, ...db.devices.map(device => device.deviceId ?? 0)) + 1
    const rawPayload = payload as Record<string, unknown>
    const typeName = typeof rawPayload.typeName === 'string' && rawPayload.typeName.length > 0
      ? rawPayload.typeName
      : typeof rawPayload.typeName === 'string'
        ? rawPayload.typeName
        : ''
    const typeId = typeof rawPayload.typeId === 'number' ? rawPayload.typeId : typeName.length > 0 ? deviceTypeIdFromName(typeName) : 0
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
    const configSource = isRecordPayload(rawPayload.config) ? rawPayload.config : {}
    const dependencyLinks = isDs18b20 ? normalizeDependencyLinks(configSource.deps) : []
    const thermostatDependencyLinks = isThermostat ? normalizeThermostatDependencyLinks(configSource.deps, configSource) : []
    const dependencyDeviceId = isDs18b20 ? dependencyDeviceIdForRole(dependencyLinks, 'onewire_bus') : 0
    if (isDs18b20) {
      requireOneWireDependency(db, dependencyDeviceId)
    }
    const enabled = typeof configSource.enabled === 'boolean' ? configSource.enabled : true
    const ds18b20Config = isDs18b20 ? normalizeDs18b20ConfigPayload(configSource, enabled) : undefined
    const thermostatConfig = isThermostat ? normalizeThermostatConfigPayload(configSource, enabled, thermostatDependencyLinks) : undefined
    if (isDs18b20 && ds18b20Config !== undefined) {
      ensureUniqueDs18b20Address(db, dependencyDeviceId, String(ds18b20Config.address), nextId)
    }
    if (isThermostat && thermostatConfig !== undefined) {
      requireThermostatDependencies(
        db,
        normalizeDependencyDeviceId(thermostatConfig.temperatureSensorDeviceId),
        normalizeDependencyDeviceId(thermostatConfig.switchDeviceId),
      )
    }

    const device: DeviceRecord = {
      deviceId: nextId,
      typeId,
      label: isGpioSwitch
        ? 'GPIO switch'
        : isOneWireBus
          ? 'OneWire bus'
          : isDs18b20
            ? 'DS18B20 temperature sensor'
            : isThermostat
              ? 'Thermostat'
              : 'Dummy device',
      typeName: isGpioSwitch
        ? 'gpio_switch'
        : isOneWireBus
          ? 'onewire_bus'
          : isDs18b20
            ? 'ds18b20_temperature_sensor'
            : isThermostat
              ? 'thermostat'
              : 'dummy',
      name: typeof configSource.name === 'string' && configSource.name.length > 0 ? configSource.name : 'New Device',
      enabled,
      deps: isThermostat ? thermostatDependencyLinks : dependencyLinks,
      hasDeps: isThermostat ? thermostatDependencyLinks.length > 0 : dependencyLinks.length > 0,
      configRevision: 1,
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      status: 'ready',
      retainedStateSupported: isGpioSwitch,
      retainedStartupEnabled: false,
      retainedStartupFallbackOutput: false,
      retainedStateInConfigPayload: false,
      config: {
        ...(isGpioSwitch
          ? {
              enabled: true,
              restorePreviousState: false,
              startupState: 'off',
              safeState: 'disabled',
              inverted: false,
              gpioPin: 2,
            }
          : isOneWireBus
            ? {
                enabled: true,
                gpioPin: 4,
                internalPullup: false,
              }
          : isDs18b20 && ds18b20Config !== undefined
            ? ds18b20Config
          : isDummy
            ? {
                enabled,
                name: typeof configSource.name === 'string' && configSource.name.length > 0 ? configSource.name : 'New Device',
              }
            : isThermostat && thermostatConfig !== undefined
              ? thermostatConfig
            : {
                enabled: true,
                restorePreviousState: false,
                defaultOutput: false,
                currentOutput: false,
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
                unitSymbol: ds18b20Config?.unit === 'fahrenheit' ? 'F' : 'C',
                measuredAtMs: 0,
                valid: false,
                status: 'not_ready',
              },
            }
          : isThermostat && thermostatConfig !== undefined
            ? buildThermostatOutput(db, thermostatConfig, nextId)
        : undefined,
      scan: isOneWireBus
        ? {
            inProgress: false,
            ready: false,
            deviceCount: 0,
            truncated: false,
            invalidCrcSeen: false,
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
    return ok({
      registryRevision: db.registryRevision,
      device: db.devices.at(-1) ? decorateDeviceRecord(db.devices.at(-1) as DeviceRecord, db.registryRevision) : undefined,
    }) as unknown as DeviceMutationResponse
  })
  const db = loadMockDatabase()
  const createdDevice = response.device ?? db.devices.at(-1)
  const deviceSnapshot = createdDevice
    ? decorateDeviceRecord(createdDevice, db.registryRevision)
    : { record: { id: 0, typeName: '', configRevision: db.registryRevision }, config: {}, runtime: {} }
  publishRealtimeMessage({
    topic: 'device.upsert',
    revision: db.registryRevision,
    payload: {
      ...deviceSnapshot,
      eventKind: 'device_created',
    },
  })
  scheduleMockPersistenceFlush()
  return Promise.resolve(response)
}

export function mockCommandDevice(deviceId: number, payload: DeviceCommandRequest): Promise<DeviceMutationResponse> {
  let removedDevice: DeviceRecord | undefined
  const response = mutateRegistry(db => {
      const device = db.devices.find(entry => entry.deviceId === deviceId)
      if (!device) {
        throw new ApiClientError('device not found', 'NOT_FOUND', 404, null)
      }

      const command = payload.command as string
      switch (command) {
        case 'rename':
          if (typeof payload.name !== 'string' || payload.name.trim().length === 0) {
            throw new ApiClientError('name is required', 'BAD_ARGS', 400, null)
          }
          device.name = payload.name
          break
        case 'enable':
          device.enabled = true
          device.lifecycleStatus = 'ready'
          device.effectiveStatus = 'ready'
          device.status = 'ready'
          if (isRecordPayload(device.config)) {
            device.config.enabled = true
          }
          break
        case 'disable':
          device.enabled = false
          device.lifecycleStatus = 'disabled'
          device.effectiveStatus = 'disabled'
          device.status = 'disabled'
          if (isRecordPayload(device.config)) {
            device.config.enabled = false
          }
          break
        case 'delete':
          removedDevice = { ...device }
          db.devices = db.devices.filter(entry => entry.deviceId !== deviceId)
          pruneDashboardLayout(db)
          break
        case 'updateConfig':
        case 'update_config':
          if (!isRecordPayload(payload.config)) {
            throw new ApiClientError('config is required', 'BAD_ARGS', 400, null)
          }
          if (device.typeId === GPIO_SWITCH_DEVICE_TYPE_ID) {
            const currentConfig = isRecordPayload(device.config) ? device.config : {}
            device.config = {
              enabled: Boolean(payload.config.enabled ?? device.enabled),
              restorePreviousState: Boolean(payload.config.restorePreviousState ?? false),
              startupState: payload.config.startupState === 'on'
                ? 'on'
                : payload.config.startupState === 'disabled'
                  ? 'disabled'
                  : 'off',
              safeState: payload.config.safeState === 'on'
                ? 'on'
                : payload.config.safeState === 'disabled'
                  ? 'disabled'
                  : 'off',
              inverted: Boolean(payload.config.inverted ?? false),
              gpioPin: normalizeFiniteNumber(payload.config.gpioPin, normalizeFiniteNumber(currentConfig.gpioPin, 2)),
            }
            device.enabled = Boolean(device.config.enabled)
          } else if (device.typeId === ONEWIRE_BUS_DEVICE_TYPE_ID) {
            const currentConfig = isRecordPayload(device.config) ? device.config : {}
            device.config = {
              enabled: Boolean(payload.config.enabled ?? device.enabled),
              gpioPin: normalizeFiniteNumber(payload.config.gpioPin, normalizeFiniteNumber(currentConfig.gpioPin, 4)),
              internalPullup: Boolean(payload.config.internalPullup ?? false),
            }
            device.enabled = Boolean(device.config.enabled)
          } else if (device.typeId === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID) {
            const dependencyLinks = normalizeDependencyLinks(payload.deps)
            const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'onewire_bus')
            if (dependencyDeviceId <= 0) {
              throw new ApiClientError('ds18b20 dependency is required', 'BAD_ARGS', 400, null)
            }
            requireOneWireDependency(db, dependencyDeviceId)
            const config = normalizeDs18b20ConfigPayload(payload.config, device.enabled)
            ensureUniqueDs18b20Address(db, dependencyDeviceId, String(config.address), device.deviceId ?? 0)
            device.config = config
            device.enabled = Boolean(config.enabled)
            device.deps = dependencyLinks
            device.hasDeps = dependencyLinks.length > 0
          } else if (device.typeId === THERMOSTAT_DEVICE_TYPE_ID) {
            const dependencyLinks = normalizeThermostatDependencyLinks(payload.deps, payload.config)
            const config = normalizeThermostatConfigPayload(payload.config, device.enabled, dependencyLinks)
            requireThermostatDependencies(
              db,
              normalizeDependencyDeviceId(config.temperatureSensorDeviceId),
              normalizeDependencyDeviceId(config.switchDeviceId),
            )
            device.config = config
            device.enabled = Boolean(config.enabled)
            device.deps = dependencyLinks
            device.hasDeps = dependencyLinks.length > 0
          }
          device.configRevision = Number(device.configRevision ?? 0) + 1
          break
        case 'setStatus':
        case 'set_status':
          if (device.typeId === DUMMY_DEVICE_TYPE_ID) {
            throw new ApiClientError('unsupported dummy command', 'BAD_ARGS', 400, null)
          }
          if (payload.status === 'fault') {
            device.lifecycleStatus = 'faulted'
            device.effectiveStatus = 'faulted'
            device.status = 'faulted'
          }
          if (payload.status === 'ready') {
            device.lifecycleStatus = 'ready'
            device.effectiveStatus = 'ready'
            device.status = 'ready'
          }
          break
        case 'scan':
          if (device.typeId !== ONEWIRE_BUS_DEVICE_TYPE_ID) {
            throw new ApiClientError('unsupported onewire command', 'BAD_ARGS', 400, null)
          }
          if (!isRecordPayload(device.config)) {
            device.config = {}
          }
          if (isRecordPayload(device.scan) && device.scan.inProgress === true) {
            throw new ApiClientError('scan already in progress', 'BAD_ARGS', 400, null)
          }
          device.scan = {
            inProgress: true,
            ready: false,
            deviceCount: 0,
            truncated: false,
            invalidCrcSeen: false,
            devices: [],
          }
          return ok({
            registryRevision: db.registryRevision,
            device,
          }) as unknown as DeviceMutationResponse
        case 'setOutput':
        case 'set_output':
          if (typeof payload.state !== 'string') {
            throw new ApiClientError('state is required', 'BAD_ARGS', 400, null)
          }
          if (device.typeId === GPIO_SWITCH_DEVICE_TYPE_ID) {
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
      return ok({
        registryRevision: db.registryRevision,
        device: db.devices.find(entry => entry.deviceId === deviceId)
          ? decorateDeviceRecord(db.devices.find(entry => entry.deviceId === deviceId) as DeviceRecord, db.registryRevision)
          : undefined,
      }) as unknown as DeviceMutationResponse
  })
  const db = loadMockDatabase()
  const deviceSnapshot = response.device
    ? decorateDeviceRecord(response.device, db.registryRevision)
    : { record: { id: deviceId, typeName: '', configRevision: db.registryRevision }, config: {}, runtime: {} }
  if (payload.command === 'delete') {
    publishRealtimeMessage({
      topic: 'device.remove',
      revision: db.registryRevision,
      payload: {
        deviceId: removedDevice?.deviceId ?? deviceId,
        eventKind: 'device_deleted',
        registryRevision: db.registryRevision,
        name: removedDevice?.name ?? '',
        typeId: removedDevice?.typeId ?? 0,
        typeName: removedDevice?.typeName ?? removedDevice?.label ?? '',
      },
    })
  } else {
    publishRealtimeMessage({
      topic: 'device.command_result',
      revision: db.registryRevision,
      payload: {
        ...deviceSnapshot,
        eventKind: 'command_accepted',
      },
    })
  }
  publishDependentThermostats(db, deviceId)
  if (
    payload.command === 'scan' &&
    response.device?.record?.typeName === 'onewire_bus' &&
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

function normalizeDependencyLinks(value: unknown): Array<{ role: string; deviceId: number }> {
  if (Array.isArray(value)) {
    return value
      .filter(isRecordPayload)
      .map(item => ({
        role: typeof item.role === 'string' ? item.role.trim() : '',
        deviceId: normalizeDependencyDeviceId(item.deviceId),
      }))
      .filter(item => item.role.length > 0 && item.deviceId > 0)
  }
  return []
}

function dependencyDeviceIdForRole(deps: Array<{ role: string; deviceId: number }>, role: string): number {
  const dependency = deps.find(link => link.role === role)
  return dependency?.deviceId ?? 0
}

function normalizeThermostatDependencyLinks(value: unknown, fallbackConfig: unknown = null): Array<{ role: string; deviceId: number }> {
  if (Array.isArray(value)) {
    const links = value
      .filter(isRecordPayload)
      .map(item => ({
        role: typeof item.role === 'string' ? item.role.trim() : '',
        deviceId: normalizeDependencyDeviceId(item.deviceId),
      }))
      .filter(item => item.role.length > 0 && item.deviceId > 0)
    if (links.length > 0) {
      return links.filter(item => item.role === 'temperature_sensor' || item.role === 'switch')
    }
  }

  if (isRecordPayload(fallbackConfig)) {
    const temperatureSensorId = normalizeDependencyDeviceId(fallbackConfig.temperatureSensorDeviceId)
    const switchDeviceId = normalizeDependencyDeviceId(fallbackConfig.switchDeviceId)
    const links: Array<{ role: string; deviceId: number }> = []
    if (temperatureSensorId > 0) {
      links.push({ role: 'temperature_sensor', deviceId: temperatureSensorId })
    }
    if (switchDeviceId > 0) {
      links.push({ role: 'switch', deviceId: switchDeviceId })
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
  deps: Array<{ role: string; deviceId: number }>,
): Record<string, unknown> {
  const current = isRecordPayload(value) ? value : {}
  const temperatureSensorId = normalizeDependencyDeviceId(current.temperatureSensorDeviceId ?? dependencyDeviceIdForRole(deps, 'temperature_sensor'))
  const switchDeviceId = normalizeDependencyDeviceId(current.switchDeviceId ?? dependencyDeviceIdForRole(deps, 'switch'))
  return {
    enabled: typeof current.enabled === 'boolean' ? current.enabled : enabledFallback,
    mode: normalizeThermostatMode(current.mode),
    algorithm: normalizeThermostatAlgorithm(current.algorithm),
    targetMilliCelsius: Math.round(normalizeFiniteNumber(current.targetMilliCelsius, 25000)),
    minSafeMilliCelsius: Math.round(normalizeFiniteNumber(current.minSafeMilliCelsius, 0)),
    maxSafeMilliCelsius: Math.round(normalizeFiniteNumber(current.maxSafeMilliCelsius, 50000)),
    hysteresisCentiCelsius: Math.max(0, Math.round(normalizeFiniteNumber(current.hysteresisCentiCelsius, 50))),
    checkIntervalMs: Math.max(250, Math.round(normalizeFiniteNumber(current.checkIntervalMs, 1000))),
    sensorTimeoutMs: Math.max(250, Math.round(normalizeFiniteNumber(current.sensorTimeoutMs, 6000))),
    retryAfterErrorMs: Math.max(250, Math.round(normalizeFiniteNumber(current.retryAfterErrorMs, 30000))),
    minSwitchIntervalMs: Math.max(0, Math.round(normalizeFiniteNumber(current.minSwitchIntervalMs, 5000))),
    temperatureSensorDeviceId: temperatureSensorId,
    switchDeviceId: switchDeviceId,
  }
}

function requireThermostatDependencies(db: ReturnType<typeof createSeedMockDatabase>, sensorDeviceId: number, switchDeviceId: number): void {
  const sensor = db.devices.find(entry => entry.deviceId === sensorDeviceId)
  if (!sensor || sensor.typeId !== DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID) {
    throw new ApiClientError('thermostat temperature sensor dependency is required', 'BAD_ARGS', 400, null)
  }
  const switchDevice = db.devices.find(entry => entry.deviceId === switchDeviceId)
  if (!switchDevice || switchDevice.typeId !== GPIO_SWITCH_DEVICE_TYPE_ID) {
    throw new ApiClientError('thermostat switch dependency is required', 'BAD_ARGS', 400, null)
  }
}

function buildThermostatOutput(
  db: ReturnType<typeof createSeedMockDatabase>,
  config: Record<string, unknown>,
  currentDeviceId: number,
): DeviceRecord['output'] {
  const sensorDeviceId = normalizeDependencyDeviceId(config.temperatureSensorDeviceId)
  const switchDeviceId = normalizeDependencyDeviceId(config.switchDeviceId)
  const sensor = db.devices.find(entry => entry.deviceId === sensorDeviceId)
  const switchDevice = db.devices.find(entry => entry.deviceId === switchDeviceId)
  const temperature = (sensor?.output as
    | { temperature?: { measuredAtMs?: number; valid?: boolean; value?: number; unit?: string; unitSymbol?: string } }
    | undefined
  )?.temperature
  const measuredAtMs = temperature?.measuredAtMs ?? 0
  const validTemperature = Boolean(temperature?.valid)
  const currentTemperature = validTemperature ? Number(temperature?.value ?? 0) : 0
  const hysteresis = Math.max(0, normalizeFiniteNumber(config.hysteresisCentiCelsius, 50)) / 100
  const target = normalizeFiniteNumber(config.targetMilliCelsius, 25000) / 1000
  const mode = normalizeThermostatMode(config.mode)
  let desiredSwitchState: 'off' | 'on' | 'disabled' = 'off'
  let controlStatus = 'ready'

  if (!Boolean(config.enabled)) {
    desiredSwitchState = 'disabled'
    controlStatus = 'disabled'
  } else if (!sensor || sensor.typeId !== DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID || !switchDevice || switchDevice.typeId !== GPIO_SWITCH_DEVICE_TYPE_ID) {
    desiredSwitchState = 'off'
    controlStatus = 'dependency_blocked'
  } else if (!sensor.enabled || sensor.effectiveStatus !== 'ready' || !validTemperature) {
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
    unitSymbol: 'C',
    measuredAtMs: 0,
    valid: false,
    status: 'not_ready',
  }

  return {
    ...((db.devices.find(entry => entry.deviceId === currentDeviceId)?.output ?? {}) as Record<string, unknown>),
    temperature: outputTemperature,
    desiredSwitchState,
    actualSwitchState,
    controlStatus,
    lastCheckAtMs: measuredAtMs,
  }
}

function requireOneWireDependency(db: ReturnType<typeof createSeedMockDatabase>, dependencyDeviceId: number): DeviceRecord {
  const dependency = db.devices.find(entry => entry.deviceId === dependencyDeviceId)
  if (!dependency || dependency.typeId !== ONEWIRE_BUS_DEVICE_TYPE_ID) {
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
    pollMs: Math.max(1000, normalizeFiniteNumber(value.pollMs, 5000)),
    reportDeltaCelsius: Math.max(0.01, normalizeFiniteNumber(value.reportDeltaCelsius, 0.01)),
    reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : false,
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
    device.deviceId !== currentDeviceId &&
    device.typeId === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID &&
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
      unitSymbol: unit === 'fahrenheit' ? 'F' : 'C',
      measuredAtMs: 0,
      valid: false,
      status,
    },
  }
}

function refreshChildEffectiveStatuses(db: ReturnType<typeof createSeedMockDatabase>): void {
  for (const device of db.devices) {
    if (device.typeId !== DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID) {
      continue
    }
    if (!device.enabled) {
      device.lifecycleStatus = 'disabled'
      device.effectiveStatus = 'disabled'
      device.status = 'disabled'
      markTemperatureUnavailable(device, 'disabled')
      continue
    }

    if (device.lifecycleStatus === 'disabled') {
      device.lifecycleStatus = 'ready'
    }
    const dependencyDeviceId = dependencyDeviceIdForRole(device.deps ?? [], 'onewire_bus')
    const dependency = db.devices.find(entry => entry.deviceId === dependencyDeviceId)
    if (!dependency || dependency.typeId !== ONEWIRE_BUS_DEVICE_TYPE_ID) {
      device.effectiveStatus = 'dependency_blocked'
      device.status = 'dependency_blocked'
      markTemperatureUnavailable(device, 'missing_dependency')
      continue
    }
    if (!dependency.enabled || dependency.effectiveStatus === 'disabled') {
      device.effectiveStatus = 'disabled'
      device.status = 'disabled'
      markTemperatureUnavailable(device, 'dependency_disabled')
      continue
    }
    if (dependency.effectiveStatus !== 'ready') {
      device.effectiveStatus = 'dependency_blocked'
      device.status = 'dependency_blocked'
      markTemperatureUnavailable(device, 'dependency_not_ready')
      continue
    }

    device.effectiveStatus = device.lifecycleStatus === 'faulted' ? 'faulted' : 'ready'
    device.status = device.effectiveStatus
    if (!(device.output as { temperature?: unknown } | undefined)?.temperature) {
      markTemperatureUnavailable(device, 'not_ready')
    }
  }

  for (const device of db.devices) {
    if (device.typeId !== THERMOSTAT_DEVICE_TYPE_ID) {
      continue
    }
    const config = isRecordPayload(device.config) ? device.config : {}
    const sensor = db.devices.find(entry => entry.deviceId === normalizeDependencyDeviceId(config.temperatureSensorDeviceId))
    const switchDevice = db.devices.find(entry => entry.deviceId === normalizeDependencyDeviceId(config.switchDeviceId))

    if (!device.enabled) {
      device.lifecycleStatus = 'disabled'
      device.effectiveStatus = 'disabled'
      device.status = 'disabled'
      device.output = buildThermostatOutput(db, config, device.deviceId ?? 0)
      continue
    }

    if (device.lifecycleStatus === 'disabled') {
      device.lifecycleStatus = 'ready'
    }
    if (!sensor || sensor.typeId !== DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID || !switchDevice || switchDevice.typeId !== GPIO_SWITCH_DEVICE_TYPE_ID) {
      device.effectiveStatus = 'dependency_blocked'
      device.status = 'dependency_blocked'
      device.output = buildThermostatOutput(db, config, device.deviceId ?? 0)
      continue
    }
    if (!sensor.enabled || sensor.effectiveStatus === 'disabled') {
      device.effectiveStatus = 'disabled'
      device.status = 'disabled'
      device.output = buildThermostatOutput(db, config, device.deviceId ?? 0)
      continue
    }

    device.effectiveStatus = device.lifecycleStatus === 'faulted' ? 'faulted' : 'ready'
    device.status = device.effectiveStatus
    device.output = buildThermostatOutput(db, config, device.deviceId ?? 0)
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
  const deviceId = Number((match as Record<string, unknown>).deviceId ?? 0)
  return Number.isFinite(deviceId) && deviceId > 0 ? deviceId : 0
}

function exportDeviceConfig(device: DeviceRecord): Record<string, unknown> {
  const config = cloneConfig(device.config)
  if (device.typeId === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID) {
    delete config.dependencyDeviceId
  }
  if (device.typeId === THERMOSTAT_DEVICE_TYPE_ID) {
    delete config.temperatureSensorDeviceId
    delete config.switchDeviceId
  }
  return config
}

export function mockExportDeviceSetupBundle(): string {
  const db = loadMockDatabase()
  const lines: string[] = []
  lines.push(JSON.stringify({
    kind: 'transfer_envelope',
    transferSchemaVersion: 1,
    registrySchemaVersion: 1,
    registryRevision: db.registryRevision,
    deviceCount: db.devices.length,
  }))

  for (const device of db.devices) {
    const config = exportDeviceConfig(device)
    lines.push(JSON.stringify({
      kind: 'device',
      id: device.deviceId,
      typeName: device.type,
      configRevision: device.configRevision,
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
    if (envelope.kind !== 'transfer_envelope' || envelope.transferSchemaVersion !== 1) {
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
      delete config.deviceId
      delete config.typeName
      delete config.typeId
      delete config.configRevision
      delete config.name
      delete config.enabled
      delete config.deps

      const typeName = typeof parsed.typeName === 'string' ? parsed.typeName : ''
      const deps = (Array.isArray(parsed.deps) ? parsed.deps : []) as NonNullable<DeviceRecord['deps']>
      if (typeName === 'ds18b20_temperature_sensor') {
        const dependencyDeviceId = dependencyIdFromExportDeps(deps, 'onewire_bus')
        if (dependencyDeviceId > 0) {
          config.dependencyDeviceId = dependencyDeviceId
        }
      }
      if (typeName === 'thermostat') {
        const temperatureSensorId = dependencyIdFromExportDeps(deps, 'temperature_sensor')
        const switchDeviceId = dependencyIdFromExportDeps(deps, 'switch')
        if (temperatureSensorId > 0) {
          config.temperatureSensorDeviceId = temperatureSensorId
        }
        if (switchDeviceId > 0) {
          config.switchDeviceId = switchDeviceId
        }
      }

      devices.push({
        deviceId: Number(parsed.id ?? parsed.deviceId ?? 0),
        typeId: Number(parsed.typeId ?? deviceTypeIdFromName(typeName)),
        label: typeof parsed.name === 'string' ? parsed.name : undefined,
        typeName,
        name: typeof parsed.name === 'string' ? parsed.name : '',
        enabled: Boolean(parsed.enabled),
        deps,
        hasDeps: deps.length > 0,
        configRevision: Number(parsed.configRevision ?? 0),
        lifecycleStatus: 'ready',
        effectiveStatus: 'ready',
        status: 'ready',
        config,
      })
    }

    mutateRegistry(db => {
      db.devices = devices
      db.registryRevision = Number(envelope.registryRevision ?? db.registryRevision)
      refreshMockDerivedDeviceState(db)
      return null
    })

    return ok({
      registryRevision: Number(envelope.registryRevision ?? 0),
      deviceCount: devices.length,
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
    schemaVersion: 1,
    activePanelId: panels.some(panel => panel.id === layout.activePanelId) ? layout.activePanelId : panels[0].id,
    panels,
  }
}

function pruneDashboardLayout(db: ReturnType<typeof createSeedMockDatabase>): void {
  db.dashboardLayout = normalizeDashboardLayout(db.dashboardLayout, db.devices.map(device => device.deviceId ?? 0))
}

function validateDashboardLayout(layout: DashboardLayoutRecord): void {
  if (layout.schemaVersion !== 1) {
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

  if (!ids.has(layout.activePanelId)) {
    throw new ApiClientError('dashboard active panel does not exist', 'INVALID_ACTIVE_PANEL', 400, null)
  }
}
