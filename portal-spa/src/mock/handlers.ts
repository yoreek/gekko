import type {
  DashboardLayoutRecord,
  DashboardLayoutWidgetRecord,
  DashboardLayoutResponse,
  DeviceCommandRequest,
  DeviceDetailResponse,
  DeviceCreateRequest,
  DeviceMutationResponse,
  MetricPlaceholderCatalogResponse,
  MetricPlaceholderDescriptor,
  MetricValuesResponse,
  DeviceSetupTransferResponse,
  DeviceRegistryResponse,
  OtaStatusResponse,
  SystemRestartResponse,
  WifiScanResponse,
  WifiStatusResponse,
} from '@/api'
import type { TemperatureOutputSnapshot } from '@/api/contracts'
import {
  defaultSsd1306Layout,
  normalizeSsd1306Layout,
} from '@/models/devices/ssd1306/layout'
import { ApiClientError } from '@/api/http'
import { publishRealtimeMessage } from '@/realtime/bus'
import { scheduleMockPersistenceFlush } from '@/realtime/mockRuntime'
import {
  canonicalizeDeviceRecord,
  createSeedMockDatabase,
  createDeviceRecord,
  loadMockDatabase,
  saveMockDatabase,
  type MockDeviceRecord,
} from './database'

type DeviceRecord = MockDeviceRecord

function ok<T extends object>(payload: T): T & { success: true } {
  return {
    success: true,
    ...payload,
  }
}

export function decorateDeviceRecord(
  device: DeviceRecord,
  _registryRevision: number,
): DeviceRecord {
  return canonicalizeDeviceRecord(device)
}

function mutateRegistry<T>(mutator: (db: ReturnType<typeof createSeedMockDatabase>) => T): T {
  const db = loadMockDatabase()
  const result = mutator(db)
  saveMockDatabase(db)
  return result
}

function completeOneWireScan(deviceId: number): void {
  const db = loadMockDatabase()
  const device = db.devices.find(entry => entry.record.id === deviceId)
  if (!device || device.record.typeName !== 'onewire_bus') {
    return
  }
  device.runtime.scan = {
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

function publishDeviceSnapshot(db: ReturnType<typeof createSeedMockDatabase>, device: DeviceRecord): void {
  publishDeviceUpsertMessage(db, device, 'snapshot')
}

function publishDependentThermostats(db: ReturnType<typeof createSeedMockDatabase>, sourceDeviceId: number): void {
  for (const device of db.devices) {
    if (device.record.typeName !== 'thermostat') {
      continue
    }
    const deps = Array.isArray(device.config.deps) ? device.config.deps : []
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

function metricDescriptor(
  namespaceName: MetricPlaceholderDescriptor['namespace'],
  sourceId: number,
  sourceLabel: string | undefined,
  metricId: number,
  metricKey: string,
  label: string,
  valueType: MetricPlaceholderDescriptor['valueType'],
  available: boolean,
  preview = '',
): MetricPlaceholderDescriptor {
  return {
    placeholder: namespaceName === 'dev' ? `{{dev.${sourceId}.${metricKey}}}` : `{{${namespaceName}.${metricKey}}}`,
    namespace: namespaceName,
    sourceId,
    sourceLabel,
    metricId,
    metricKey,
    label,
    valueType,
    available,
    preview,
  }
}

export function mockFetchMetricPlaceholders(): MetricPlaceholderCatalogResponse {
  const db = loadMockDatabase()
  const placeholders: MetricPlaceholderDescriptor[] = []
  for (const device of db.devices) {
    const label = typeof device.config.name === 'string' && device.config.name.length > 0 ? device.config.name : `Device ${device.record.id}`
    if (device.record.typeName === 'ds18b20_temperature_sensor') {
      const temperature = (device.runtime.output as { temperature?: TemperatureOutputSnapshot } | undefined)?.temperature
      placeholders.push(metricDescriptor(
        'dev',
        device.record.id,
        label,
        100,
        'temperature',
        `${label} temperature`,
        'float',
        Boolean(temperature?.valid),
        temperature?.valid ? `${temperature.value.toFixed(2)} ${temperature.unitSymbol}` : '',
      ))
    }
    if (device.record.typeName === 'gpio_switch') {
      const state = (device.runtime.output as { state?: string } | undefined)?.state
      placeholders.push(metricDescriptor('dev', device.record.id, label, 200, 'state', `${label} state`, 'string', Boolean(state), state ?? ''))
    }
  }
  placeholders.push(metricDescriptor('system', 0, undefined, 1, 'time', 'System time', 'string', true, '0:00'))
  placeholders.push(metricDescriptor('system', 0, undefined, 2, 'uptime', 'System uptime', 'int', true, '0 ms'))
  placeholders.push(metricDescriptor('system', 0, undefined, 3, 'wifi.station_ip', 'WiFi station IP', 'string', db.wifi.stationIp.length > 0, db.wifi.stationIp))
  placeholders.push(metricDescriptor('system', 0, undefined, 4, 'wifi.setup_ap_ip', 'WiFi AP IP', 'string', db.wifi.setupApIp.length > 0, db.wifi.setupApIp))
  return ok({
    registryRevision: db.registryRevision,
    placeholders,
  })
}

export function mockFetchMetricValues(): MetricValuesResponse {
  const catalog = mockFetchMetricPlaceholders()
  return ok({
    registryRevision: catalog.registryRevision,
    values: catalog.placeholders.map(metric => ({
      namespace: metric.namespace,
      sourceId: metric.sourceId,
      sourceLabel: metric.sourceLabel,
      metricId: metric.metricId,
      metricKey: metric.metricKey,
      valueType: metric.valueType,
      available: metric.available,
      value: metric.preview ?? '',
    })),
  })
}

export function mockFetchDevice(deviceId: number): DeviceDetailResponse {
  const db = loadMockDatabase()
  const device = db.devices.find(entry => entry.record.id === deviceId)
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
    db.dashboardLayout = normalizeDashboardLayout(layout, db.devices.map(device => device.record.id))
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
    const nextId = Math.max(1, ...db.devices.map(device => device.record.id)) + 1
    const rawPayload = payload as Record<string, unknown>
    const typeName = typeof rawPayload.typeName === 'string' && rawPayload.typeName.length > 0
      ? rawPayload.typeName
      : ''
    if (
      typeName !== 'dummy' &&
      typeName !== 'gpio_switch' &&
      typeName !== 'onewire_bus' &&
      typeName !== 'i2c_bus' &&
      typeName !== 'ssd1306' &&
      typeName !== 'st7735' &&
      typeName !== 'ds18b20_temperature_sensor' &&
      typeName !== 'thermostat'
    ) {
      throw new ApiClientError('unsupported device type', 'UNSUPPORTED_TYPE', 400, null)
    }

    const configSource = isRecordPayload(rawPayload.config) ? rawPayload.config : {}
    const baseDeps = Array.isArray(configSource.deps) ? normalizeDependencyLinks(configSource.deps) : []
    const enabled = typeof configSource.enabled === 'boolean' ? configSource.enabled : true
    const name = typeof configSource.name === 'string' && configSource.name.length > 0 ? configSource.name : 'New Device'

    let device: DeviceRecord

    if (typeName === 'gpio_switch') {
      const config = {
        enabled,
        name,
        deps: baseDeps,
        restorePreviousState: typeof configSource.restorePreviousState === 'boolean' ? configSource.restorePreviousState : false,
        startupState: configSource.startupState === 'on'
          ? 'on'
          : configSource.startupState === 'disabled'
            ? 'disabled'
            : 'off',
        safeState: configSource.safeState === 'on'
          ? 'on'
          : configSource.safeState === 'disabled'
            ? 'disabled'
            : 'off',
        inverted: typeof configSource.inverted === 'boolean' ? configSource.inverted : false,
        gpioPin: normalizeFiniteNumber(configSource.gpioPin, 2),
      }
      device = createDeviceRecord(nextId, typeName, 1, config, {
        status: 'ready',
        lifecycleStatus: 'ready',
        effectiveStatus: 'ready',
        output: {
          state: 'off',
        },
      })
    } else if (typeName === 'onewire_bus') {
      const config = {
        enabled,
        name,
        deps: baseDeps,
        gpioPin: normalizeFiniteNumber(configSource.gpioPin, 4),
        internalPullup: typeof configSource.internalPullup === 'boolean' ? configSource.internalPullup : false,
      }
      device = createDeviceRecord(nextId, typeName, 1, config, {
        status: 'ready',
        lifecycleStatus: 'ready',
        effectiveStatus: 'ready',
        scan: {
          inProgress: false,
          ready: false,
          deviceCount: 0,
          truncated: false,
          invalidCrcSeen: false,
          devices: [],
        },
      })
    } else if (typeName === 'i2c_bus') {
      const config = normalizeI2cBusConfigPayload(configSource, enabled)
      device = createDeviceRecord(nextId, typeName, 1, {
        ...config,
        name,
        deps: baseDeps,
      }, {
        status: 'ready',
        lifecycleStatus: 'ready',
        effectiveStatus: 'ready',
        generation: 1,
        transactionActive: false,
      })
    } else if (typeName === 'ssd1306') {
      const dependencyDeviceId = dependencyDeviceIdForRole(baseDeps, 'i2c_bus') || normalizeDependencyDeviceId(configSource.i2cBusDeviceId)
      if (dependencyDeviceId <= 0) {
        throw new ApiClientError('ssd1306 display i2c dependency is required', 'BAD_ARGS', 400, null)
      }
      requireI2cDependency(db, dependencyDeviceId)
      const i2cAddress = normalizeFiniteNumber(configSource.i2cAddress, 60)
      ensureUniqueI2cAddress(db, dependencyDeviceId, i2cAddress, nextId)
      const layout = isRecordPayload(configSource.layout)
        ? normalizeSsd1306Layout(configSource.layout)
        : defaultSsd1306Layout()
      const config = {
        enabled,
        name,
        deps: [
          {
            role: 'i2c_bus',
            deviceId: dependencyDeviceId,
          },
        ],
        i2cBusDeviceId: dependencyDeviceId,
        i2cAddress,
        width: normalizeFiniteNumber(configSource.width, 128),
        height: normalizeFiniteNumber(configSource.height, 64),
        layout,
      }
      device = createDeviceRecord(nextId, typeName, 1, config, {
        status: 'ready',
        lifecycleStatus: 'ready',
        effectiveStatus: 'ready',
      })
    } else if (typeName === 'st7735') {
      const dependencyDeviceId = dependencyDeviceIdForRole(baseDeps, 'spi_bus') || normalizeDependencyDeviceId(configSource.spiBusDeviceId)
      if (dependencyDeviceId <= 0) {
        throw new ApiClientError('st7735 display spi dependency is required', 'BAD_ARGS', 400, null)
      }
      requireSpiDependency(db, dependencyDeviceId)
      const layoutRaw = isRecordPayload(configSource.layout) ? configSource.layout : {}
      const config = {
        enabled,
        name,
        deps: [
          {
            role: 'spi_bus',
            deviceId: dependencyDeviceId,
          },
        ],
        spiBusDeviceId: dependencyDeviceId,
        chipSelectPin: normalizeFiniteNumber(configSource.chipSelectPin, 5),
        dcPin: normalizeFiniteNumber(configSource.dcPin, 2),
        resetPin: normalizeFiniteNumber(configSource.resetPin, -1),
        width: normalizeFiniteNumber(configSource.width, 128),
        height: normalizeFiniteNumber(configSource.height, 160),
        layout: {
          schemaVersion: 1,
          activePageId: typeof layoutRaw.activePageId === 'string' ? layoutRaw.activePageId : 'main',
          pages: Array.isArray(layoutRaw.pages) ? layoutRaw.pages : [],
          colorMode: 'rgb565',
        },
      }
      device = createDeviceRecord(nextId, typeName, 1, config, {
        status: 'ready',
        lifecycleStatus: 'ready',
        effectiveStatus: 'ready',
      })
    } else if (typeName === 'ds18b20_temperature_sensor') {
      const deps = normalizeDependencyLinks(configSource.deps)
      const dependencyDeviceId = dependencyDeviceIdForRole(deps, 'onewire_bus')
      if (dependencyDeviceId <= 0) {
        throw new ApiClientError('ds18b20 dependency is required', 'BAD_ARGS', 400, null)
      }
      requireOneWireDependency(db, dependencyDeviceId)
      const config = normalizeDs18b20ConfigPayload(configSource, enabled)
      ensureUniqueDs18b20Address(db, dependencyDeviceId, String(config.address), nextId)
      device = createDeviceRecord(nextId, typeName, 1, {
        ...config,
        deps,
        name,
      }, {
        status: 'ready',
        lifecycleStatus: 'ready',
        effectiveStatus: 'ready',
        output: {
          temperature: {
            value: 0,
            unit: config.unit === 'fahrenheit' ? 'fahrenheit' : 'celsius',
            unitSymbol: config.unit === 'fahrenheit' ? 'F' : 'C',
            measuredAtMs: 0,
            valid: false,
            status: 'not_ready',
          },
        },
      })
    } else if (typeName === 'thermostat') {
      const deps = normalizeThermostatDependencyLinks(configSource.deps, configSource)
      const config = normalizeThermostatConfigPayload(configSource, enabled)
      requireThermostatDependencies(db, deps)
      device = createDeviceRecord(nextId, typeName, 1, {
        ...config,
        deps,
        name,
      }, {
        status: 'ready',
        lifecycleStatus: 'ready',
        effectiveStatus: 'ready',
        output: buildThermostatOutput(db, config, nextId),
      })
    } else {
      const config = {
        enabled,
        name,
        deps: baseDeps,
      }
      device = createDeviceRecord(nextId, typeName, 1, config, {
        status: 'ready',
        lifecycleStatus: 'ready',
        effectiveStatus: 'ready',
      })
    }

    db.devices.push(device)
    refreshChildEffectiveStatuses(db)
    db.registryRevision += 1
    return ok({
      registryRevision: db.registryRevision,
      device: decorateDeviceRecord(device, db.registryRevision),
    }) as unknown as DeviceMutationResponse
  })
  const db = loadMockDatabase()
  const createdDevice = (response.device as DeviceRecord | undefined) ?? db.devices.at(-1)
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
    const device = db.devices.find(entry => entry.record.id === deviceId)
    if (!device) {
      throw new ApiClientError('device not found', 'NOT_FOUND', 404, null)
    }

    const command = payload.command as string
    switch (command) {
      case 'rename':
        if (typeof payload.name !== 'string' || payload.name.trim().length === 0) {
          throw new ApiClientError('name is required', 'BAD_ARGS', 400, null)
        }
        device.config.name = payload.name
        break
      case 'enable':
        device.config.enabled = true
        device.runtime.lifecycleStatus = 'ready'
        device.runtime.effectiveStatus = 'ready'
        device.runtime.status = 'ready'
        break
      case 'disable':
        device.config.enabled = false
        device.runtime.lifecycleStatus = 'disabled'
        device.runtime.effectiveStatus = 'disabled'
        device.runtime.status = 'disabled'
        break
      case 'delete':
        removedDevice = { ...device }
        db.devices = db.devices.filter(entry => entry.record.id !== deviceId)
        pruneDashboardLayout(db)
        break
      case 'updateConfig':
      case 'update_config':
        if (!isRecordPayload(payload.config)) {
          throw new ApiClientError('config is required', 'BAD_ARGS', 400, null)
        }
        if (device.record.typeName === 'gpio_switch') {
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: device.config.deps,
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
            gpioPin: normalizeFiniteNumber(payload.config.gpioPin, normalizeFiniteNumber(currentConfig['gpioPin'], 2)),
          }
          device.runtime.lifecycleStatus = 'ready'
          device.runtime.effectiveStatus = 'ready'
          device.runtime.status = 'ready'
        } else if (device.record.typeName === 'onewire_bus') {
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: device.config.deps,
            gpioPin: normalizeFiniteNumber(payload.config.gpioPin, normalizeFiniteNumber(currentConfig['gpioPin'], 4)),
            internalPullup: Boolean(payload.config.internalPullup ?? false),
          }
          device.runtime.lifecycleStatus = 'ready'
          device.runtime.effectiveStatus = 'ready'
          device.runtime.status = 'ready'
        } else if (device.record.typeName === 'i2c_bus') {
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: device.config.deps,
            sdaPin: normalizeFiniteNumber(payload.config.sdaPin, normalizeFiniteNumber(currentConfig['sdaPin'], 21)),
            sclPin: normalizeFiniteNumber(payload.config.sclPin, normalizeFiniteNumber(currentConfig['sclPin'], 22)),
            internalPullup: Boolean(payload.config.internalPullup ?? true),
            frequencyHz: Math.max(1, Math.round(normalizeFiniteNumber(payload.config.frequencyHz, normalizeFiniteNumber(currentConfig['frequencyHz'], 100000)))),
          }
          device.runtime.lifecycleStatus = 'ready'
          device.runtime.effectiveStatus = 'ready'
          device.runtime.status = 'ready'
          device.runtime.transactionActive = false
          device.runtime.generation = normalizeFiniteNumber(device.runtime.generation, 0) + 1
        } else if (device.record.typeName === 'ssd1306') {
          const dependencyLinks = normalizeDependencyLinks(payload.deps ?? device.config.deps)
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'i2c_bus')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError('ssd1306 display i2c dependency is required', 'BAD_ARGS', 400, null)
          }
          requireI2cDependency(db, dependencyDeviceId)
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          const i2cAddress = normalizeFiniteNumber(payload.config.i2cAddress, normalizeFiniteNumber(currentConfig['i2cAddress'], 60))
          ensureUniqueI2cAddress(db, dependencyDeviceId, i2cAddress, device.record.id)
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
            i2cBusDeviceId: dependencyDeviceId,
            i2cAddress,
            width: normalizeFiniteNumber(payload.config.width, normalizeFiniteNumber(currentConfig['width'], 128)),
            height: normalizeFiniteNumber(payload.config.height, normalizeFiniteNumber(currentConfig['height'], 64)),
            layout: isRecordPayload(payload.config.layout)
              ? normalizeSsd1306Layout(payload.config.layout)
              : normalizeSsd1306Layout(device.config.layout),
          }
        } else if (device.record.typeName === 'st7735') {
          const dependencyLinks = normalizeDependencyLinks(payload.deps ?? device.config.deps)
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'spi_bus')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError('st7735 display spi dependency is required', 'BAD_ARGS', 400, null)
          }
          requireSpiDependency(db, dependencyDeviceId)
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
            spiBusDeviceId: dependencyDeviceId,
            chipSelectPin: normalizeFiniteNumber(payload.config.chipSelectPin, normalizeFiniteNumber(currentConfig['chipSelectPin'], 5)),
            dcPin: normalizeFiniteNumber(payload.config.dcPin, normalizeFiniteNumber(currentConfig['dcPin'], 2)),
            resetPin: normalizeFiniteNumber(payload.config.resetPin, normalizeFiniteNumber(currentConfig['resetPin'], -1)),
            width: normalizeFiniteNumber(payload.config.width, normalizeFiniteNumber(currentConfig['width'], 128)),
            height: normalizeFiniteNumber(payload.config.height, normalizeFiniteNumber(currentConfig['height'], 160)),
            layout: isRecordPayload(payload.config.layout)
              ? {
                  schemaVersion: 1,
                  activePageId: typeof payload.config.layout.activePageId === 'string' ? payload.config.layout.activePageId : 'main',
                  pages: Array.isArray(payload.config.layout.pages) ? payload.config.layout.pages : [],
                  colorMode: 'rgb565',
                }
              : device.config.layout,
          }
        } else if (device.record.typeName === 'ds18b20_temperature_sensor') {
          const dependencyLinks = normalizeDependencyLinks(payload.deps ?? device.config.deps)
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'onewire_bus')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError('ds18b20 dependency is required', 'BAD_ARGS', 400, null)
          }
          requireOneWireDependency(db, dependencyDeviceId)
          const config = normalizeDs18b20ConfigPayload(payload.config, device.config.enabled)
          ensureUniqueDs18b20Address(db, dependencyDeviceId, String(config.address), device.record.id)
          device.config = {
            ...config,
            name: typeof config.name === 'string' && config.name.length > 0 ? config.name : device.config.name,
            deps: dependencyLinks,
          }
          device.runtime.output = {
            ...(isRecordPayload(device.runtime.output) ? device.runtime.output : {}),
            temperature: {
              value: 0,
              unit: config.unit === 'fahrenheit' ? 'fahrenheit' : 'celsius',
              unitSymbol: config.unit === 'fahrenheit' ? 'F' : 'C',
              measuredAtMs: 0,
              valid: false,
              status: 'not_ready',
            },
          }
        } else if (device.record.typeName === 'thermostat') {
          const dependencyLinks = normalizeThermostatDependencyLinks(payload.deps ?? device.config.deps, payload.config)
          const config = normalizeThermostatConfigPayload(payload.config, device.config.enabled)
          requireThermostatDependencies(db, dependencyLinks)
          device.config = {
            ...config,
            name: typeof config.name === 'string' && config.name.length > 0 ? config.name : device.config.name,
            deps: dependencyLinks,
          }
          device.runtime.output = buildThermostatOutput(db, device.config, device.record.id)
        }
        device.record.configRevision += 1
        break
      case 'setDeps':
      case 'set_deps': {
        const dependencyLinks = normalizeDependencyLinks(payload.deps)
        if (device.record.typeName === 'ssd1306') {
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'i2c_bus')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError(`${device.record.typeName} display i2c dependency is required`, 'BAD_ARGS', 400, null)
          }
          requireI2cDependency(db, dependencyDeviceId)
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          ensureUniqueI2cAddress(db, dependencyDeviceId, normalizeFiniteNumber(currentConfig['i2cAddress'], 60), device.record.id)
          device.config = {
            ...device.config,
            deps: dependencyLinks,
            i2cBusDeviceId: dependencyDeviceId,
          }
        } else if (device.record.typeName === 'st7735') {
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'spi_bus')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError('st7735 display spi dependency is required', 'BAD_ARGS', 400, null)
          }
          requireSpiDependency(db, dependencyDeviceId)
          device.config = {
            ...device.config,
            deps: dependencyLinks,
            spiBusDeviceId: dependencyDeviceId,
          }
        } else if (device.record.typeName === 'ds18b20_temperature_sensor') {
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'onewire_bus')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError('ds18b20 dependency is required', 'BAD_ARGS', 400, null)
          }
          requireOneWireDependency(db, dependencyDeviceId)
          device.config = {
            ...device.config,
            deps: dependencyLinks,
          }
        } else if (device.record.typeName === 'thermostat') {
          requireThermostatDependencies(db, dependencyLinks)
          device.config = {
            ...device.config,
            deps: dependencyLinks,
          }
        } else {
          device.config = {
            ...device.config,
            deps: dependencyLinks,
          }
        }
        device.record.configRevision += 1
        break
      }
      case 'setStatus':
      case 'set_status':
        if (device.record.typeName === 'dummy') {
          throw new ApiClientError('unsupported dummy command', 'BAD_ARGS', 400, null)
        }
        if (payload.status === 'fault') {
          device.runtime.lifecycleStatus = 'faulted'
          device.runtime.effectiveStatus = 'faulted'
          device.runtime.status = 'faulted'
        }
        if (payload.status === 'ready') {
          device.runtime.lifecycleStatus = 'ready'
          device.runtime.effectiveStatus = 'ready'
          device.runtime.status = 'ready'
        }
        break
      case 'resetDiagnostics':
        if (isRecordPayload(device.runtime.diagnostics)) {
          device.runtime.diagnostics = {
            status: 'ok',
            consecutiveErrors: 0,
            lastErrorCode: 0,
            lastErrorAtMs: 0,
            errorOps: 0,
          }
        }
        break
      case 'scan':
        if (device.record.typeName === 'onewire_bus') {
          if (isRecordPayload(device.runtime.scan) && device.runtime.scan.inProgress === true) {
            throw new ApiClientError('scan already in progress', 'BAD_ARGS', 400, null)
          }
          device.runtime.scan = {
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
        }
        if (device.record.typeName === 'i2c_bus') {
          device.runtime.scan = {
            inProgress: false,
            ready: true,
            deviceCount: 2,
            truncated: false,
            nextAddress: 0x77,
            devices: [
              { address: 0x3C, addressHex: '0x3C' },
              { address: 0x68, addressHex: '0x68' },
            ],
          }
          break
        }
        throw new ApiClientError('unsupported scan command', 'BAD_ARGS', 400, null)
      case 'setOutput':
      case 'set_output':
        if (typeof payload.state !== 'string') {
          throw new ApiClientError('state is required', 'BAD_ARGS', 400, null)
        }
        if (device.record.typeName === 'gpio_switch') {
          if (payload.state !== 'on' && payload.state !== 'off' && payload.state !== 'disabled') {
            throw new ApiClientError('unsupported gpio switch output state', 'BAD_ARGS', 400, null)
          }
          device.runtime.output = {
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
      device: decorateDeviceRecord(device, db.registryRevision),
    }) as unknown as DeviceMutationResponse
  })
  const db = loadMockDatabase()
  const deviceSnapshot = response.device
    ? decorateDeviceRecord(response.device as DeviceRecord, db.registryRevision)
    : { record: { id: deviceId, typeName: '', configRevision: db.registryRevision }, config: {}, runtime: {} }
  if (payload.command === 'delete') {
    publishRealtimeMessage({
      topic: 'device.remove',
      revision: db.registryRevision,
      payload: removedDevice ? decorateDeviceRecord(removedDevice, db.registryRevision) : { record: { id: deviceId, typeName: '', configRevision: db.registryRevision }, config: {}, runtime: {} },
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
): Record<string, unknown> & { enabled: boolean } {
  const current = isRecordPayload(value) ? value : {}
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
  }
}

function requireThermostatDependencies(db: ReturnType<typeof createSeedMockDatabase>, deps: Array<{ role: string; deviceId: number }>): void {
  const sensorDeviceId = dependencyDeviceIdForRole(deps, 'temperature_sensor')
  const switchDeviceId = dependencyDeviceIdForRole(deps, 'switch')
  const sensor = db.devices.find(entry => entry.record.id === sensorDeviceId)
  if (!sensor || sensor.record.typeName !== 'ds18b20_temperature_sensor') {
    throw new ApiClientError('thermostat temperature sensor dependency is required', 'BAD_ARGS', 400, null)
  }
  const switchDevice = db.devices.find(entry => entry.record.id === switchDeviceId)
  if (!switchDevice || switchDevice.record.typeName !== 'gpio_switch') {
    throw new ApiClientError('thermostat switch dependency is required', 'BAD_ARGS', 400, null)
  }
}

function buildThermostatOutput(
  db: ReturnType<typeof createSeedMockDatabase>,
  config: Record<string, unknown>,
  currentDeviceId: number,
): Record<string, unknown> {
  const deps = Array.isArray(config.deps) ? config.deps : []
  const sensorDeviceId = dependencyDeviceIdForRole(deps as Array<{ role: string; deviceId: number }>, 'temperature_sensor')
  const switchDeviceId = dependencyDeviceIdForRole(deps as Array<{ role: string; deviceId: number }>, 'switch')
  const sensor = db.devices.find(entry => entry.record.id === sensorDeviceId)
  const switchDevice = db.devices.find(entry => entry.record.id === switchDeviceId)
  const temperature = (sensor?.runtime.output as
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
  } else if (!sensor || sensor.record.typeName !== 'ds18b20_temperature_sensor' || !switchDevice || switchDevice.record.typeName !== 'gpio_switch') {
    desiredSwitchState = 'off'
    controlStatus = 'dependency_blocked'
  } else if (!sensor.config.enabled || sensor.runtime.effectiveStatus !== 'ready' || !validTemperature) {
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
      desiredSwitchState = ((switchDevice.runtime.output as { state?: 'off' | 'on' | 'disabled' } | undefined)?.state === 'on' ? 'on' : 'off')
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
      desiredSwitchState = ((switchDevice.runtime.output as { state?: 'off' | 'on' | 'disabled' } | undefined)?.state === 'on' ? 'on' : 'off')
      controlStatus = desiredSwitchState === 'on' ? 'cooling' : 'idle'
    }
  }

  const actualSwitchState = (switchDevice?.runtime.output as { state?: 'off' | 'on' | 'disabled' } | undefined)?.state ?? 'off'
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
    ...((db.devices.find(entry => entry.record.id === currentDeviceId)?.runtime.output ?? {}) as Record<string, unknown>),
    temperature: outputTemperature,
    desiredSwitchState,
    actualSwitchState,
    controlStatus,
    lastCheckAtMs: measuredAtMs,
  }
}

function requireOneWireDependency(db: ReturnType<typeof createSeedMockDatabase>, dependencyDeviceId: number): DeviceRecord {
  const dependency = db.devices.find(entry => entry.record.id === dependencyDeviceId)
  if (!dependency || dependency.record.typeName !== 'onewire_bus') {
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

function normalizeI2cFrequency(value: unknown): number {
  return Math.max(1, Math.round(normalizeFiniteNumber(value, 100000)))
}

function normalizeI2cBusConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid i2c config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    sdaPin: normalizeFiniteNumber(value.sdaPin, 21),
    sclPin: normalizeFiniteNumber(value.sclPin, 22),
    internalPullup: typeof value.internalPullup === 'boolean' ? value.internalPullup : true,
    frequencyHz: normalizeI2cFrequency(value.frequencyHz),
  }
}

function normalizeDs18b20ConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
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
    device.record.id !== currentDeviceId &&
    device.record.typeName === 'ds18b20_temperature_sensor' &&
    dependencyDeviceIdForRole((device.config.deps ?? []) as Array<{ role: string; deviceId: number }>, 'onewire_bus') === dependencyDeviceId &&
    typeof device.config.address === 'string' &&
    device.config.address.trim().toUpperCase() === normalizedAddress
  ))
  if (duplicate) {
    throw new ApiClientError('ds18b20 address already exists on this dependency', 'DUPLICATE_ADDRESS', 400, null)
  }
}

function requireI2cDependency(db: ReturnType<typeof createSeedMockDatabase>, dependencyDeviceId: number): DeviceRecord {
  const dependency = db.devices.find(device => device.record.id === dependencyDeviceId)
  if (!dependency || dependency.record.typeName !== 'i2c_bus') {
    throw new ApiClientError('ssd1306 display i2c dependency is required', 'BAD_ARGS', 400, null)
  }
  return dependency
}

function requireSpiDependency(db: ReturnType<typeof createSeedMockDatabase>, dependencyDeviceId: number): DeviceRecord {
  const dependency = db.devices.find(device => device.record.id === dependencyDeviceId)
  if (!dependency || dependency.record.typeName !== 'spi_bus') {
    throw new ApiClientError('st7735 display spi dependency is required', 'BAD_ARGS', 400, null)
  }
  return dependency
}

function ensureUniqueI2cAddress(
  db: ReturnType<typeof createSeedMockDatabase>,
  dependencyDeviceId: number,
  address: number,
  currentDeviceId: number,
): void {
  const duplicate = db.devices.some(device => (
    device.record.id !== currentDeviceId &&
    device.record.typeName === 'ssd1306' &&
    dependencyDeviceIdForRole((device.config.deps ?? []) as Array<{ role: string; deviceId: number }>, 'i2c_bus') === dependencyDeviceId &&
    normalizeFiniteNumber((device.config as Record<string, unknown>).i2cAddress, -1) === address
  ))
  if (duplicate) {
    throw new ApiClientError('ssd1306 display i2c address already exists on this dependency', 'DUPLICATE_ADDRESS', 400, null)
  }
}

function markTemperatureUnavailable(device: DeviceRecord, status: string): void {
  const unit = device.config.unit === 'fahrenheit' ? 'fahrenheit' : 'celsius'
  device.runtime.output = {
    ...(device.runtime.output ?? {}),
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
    if (device.record.typeName !== 'ds18b20_temperature_sensor') {
      continue
    }
    if (!device.config.enabled) {
      device.runtime.lifecycleStatus = 'disabled'
      device.runtime.effectiveStatus = 'disabled'
      device.runtime.status = 'disabled'
      markTemperatureUnavailable(device, 'disabled')
      continue
    }

    if (device.runtime.lifecycleStatus === 'disabled') {
      device.runtime.lifecycleStatus = 'ready'
    }
    const dependencyDeviceId = dependencyDeviceIdForRole(device.config.deps ?? [], 'onewire_bus')
    const dependency = db.devices.find(entry => entry.record.id === dependencyDeviceId)
    if (!dependency || dependency.record.typeName !== 'onewire_bus') {
      device.runtime.effectiveStatus = 'dependency_blocked'
      device.runtime.status = 'dependency_blocked'
      markTemperatureUnavailable(device, 'missing_dependency')
      continue
    }
    if (!dependency.config.enabled || dependency.runtime.effectiveStatus === 'disabled') {
      device.runtime.effectiveStatus = 'disabled'
      device.runtime.status = 'disabled'
      markTemperatureUnavailable(device, 'dependency_disabled')
      continue
    }
    if (dependency.runtime.effectiveStatus !== 'ready') {
      device.runtime.effectiveStatus = 'dependency_blocked'
      device.runtime.status = 'dependency_blocked'
      markTemperatureUnavailable(device, 'dependency_not_ready')
      continue
    }

    device.runtime.effectiveStatus = device.runtime.lifecycleStatus === 'faulted' ? 'faulted' : 'ready'
    device.runtime.status = device.runtime.effectiveStatus
    if (!(device.runtime.output as { temperature?: unknown } | undefined)?.temperature) {
      markTemperatureUnavailable(device, 'not_ready')
    }
  }

  for (const device of db.devices) {
    if (device.record.typeName !== 'thermostat') {
      continue
    }
    const config = device.config as Record<string, unknown>
    const sensorDeviceId = dependencyDeviceIdForRole((config.deps ?? []) as Array<{ role: string; deviceId: number }>, 'temperature_sensor')
    const switchDeviceId = dependencyDeviceIdForRole((config.deps ?? []) as Array<{ role: string; deviceId: number }>, 'switch')
    const sensor = db.devices.find(entry => entry.record.id === sensorDeviceId)
    const switchDevice = db.devices.find(entry => entry.record.id === switchDeviceId)

    if (!device.config.enabled) {
      device.runtime.lifecycleStatus = 'disabled'
      device.runtime.effectiveStatus = 'disabled'
      device.runtime.status = 'disabled'
      device.runtime.output = buildThermostatOutput(db, config, device.record.id)
      continue
    }

    if (device.runtime.lifecycleStatus === 'disabled') {
      device.runtime.lifecycleStatus = 'ready'
    }
    if (!sensor || sensor.record.typeName !== 'ds18b20_temperature_sensor' || !switchDevice || switchDevice.record.typeName !== 'gpio_switch') {
      device.runtime.effectiveStatus = 'dependency_blocked'
      device.runtime.status = 'dependency_blocked'
      device.runtime.output = buildThermostatOutput(db, config, device.record.id)
      continue
    }
    if (!sensor.config.enabled || sensor.runtime.effectiveStatus === 'disabled') {
      device.runtime.effectiveStatus = 'disabled'
      device.runtime.status = 'disabled'
      device.runtime.output = buildThermostatOutput(db, config, device.record.id)
      continue
    }

    device.runtime.effectiveStatus = device.runtime.lifecycleStatus === 'faulted' ? 'faulted' : 'ready'
    device.runtime.status = device.runtime.effectiveStatus
    device.runtime.output = buildThermostatOutput(db, config, device.record.id)
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

function exportDeviceConfig(device: DeviceRecord): Record<string, unknown> {
  const config = cloneConfig(device.config)
  if (device.record.typeName === 'ds18b20_temperature_sensor') {
    delete config.dependencyDeviceId
  }
  if (device.record.typeName === 'thermostat') {
    delete config.temperatureSensorDeviceId
    delete config.switchDeviceId
  }
  return config
}

interface DeviceSetupExportRecord {
  record: DeviceRecord['record']
  config: Record<string, unknown>
}

function exportDeviceRecord(device: DeviceRecord): DeviceSetupExportRecord {
  return {
    record: {
      id: device.record.id,
      typeName: device.record.typeName,
      configRevision: device.record.configRevision,
    },
    config: exportDeviceConfig(device),
  }
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
    lines.push(JSON.stringify({
      kind: 'device',
      ...exportDeviceRecord(device),
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
      if (!isRecordPayload(parsed.record) || !isRecordPayload(parsed.config)) {
        throw new ApiClientError('device bundle record must include record and config', 'BAD_ARGS', 400, null)
      }
      const recordSource = parsed.record
      const configSource = parsed.config
      const config = cloneConfig(configSource)
      delete config.kind
      delete config.record
      delete config.id
      delete config.deviceId
      delete config.typeName
      delete config.configRevision
      delete config.config

      const typeName = typeof recordSource.typeName === 'string' ? recordSource.typeName : ''
      const deps = normalizeDependencyLinks(configSource.deps)
      const enabled = typeof configSource.enabled === 'boolean' ? configSource.enabled : true
      const name = typeof configSource.name === 'string' ? configSource.name : ''
      devices.push(createDeviceRecord(
        Number(recordSource.id ?? 0),
        typeName,
        Number(recordSource.configRevision ?? 0),
        {
          ...config,
          name,
          enabled,
          deps,
        },
        {
          status: 'ready',
          lifecycleStatus: 'ready',
          effectiveStatus: 'ready',
        },
      ))
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
  const source = layout as unknown as Record<string, unknown>
  const allowedIds = new Set(deviceIds)
  const rawPanels = Array.isArray(source.panels) ? (source.panels as Array<Record<string, unknown>>) : []
  const panels = rawPanels.slice(0, 8).map((panel, index) => ({
    id: String(panel.id ?? `panel-${index + 1}`),
    name: String(panel.name ?? 'Panel').slice(0, 32),
    order: index,
    widgets: (Array.isArray(panel.widgets) ? panel.widgets : [])
      .map(widget => Array.isArray(widget) ? widget : null)
      .filter((widget): widget is Array<unknown> => widget !== null && widget.length >= 5)
      .map(widget => [
        Number(widget[0]),
        Math.max(0, Number(widget[1])),
        Math.max(0, Number(widget[2])),
        Math.max(1, Number(widget[3])),
        Math.max(1, Number(widget[4])),
      ] as DashboardLayoutWidgetRecord)
      .filter(widget => allowedIds.has(widget[0])),
  }))
  if (panels.length === 0) {
    panels.push({
      id: 'main',
      name: 'Main panel',
      order: 0,
      widgets: deviceIds.map((deviceId, index) => [deviceId, index % 6, Math.floor(index / 6), 1, 1]),
    })
  }
  const activePanelId = String(source.active_panel_id ?? source.activePanelId ?? 'main')
  return {
    schemaVersion: 1,
    activePanelId: panels.some(panel => panel.id === activePanelId) ? activePanelId : panels[0].id,
    panels,
  }
}

function pruneDashboardLayout(db: ReturnType<typeof createSeedMockDatabase>): void {
  db.dashboardLayout = normalizeDashboardLayout(db.dashboardLayout, db.devices.map(device => device.record.id))
}

function validateDashboardLayout(layout: DashboardLayoutRecord): void {
  const source = layout as unknown as Record<string, unknown>
  const schemaVersion = Number(source.schema_version ?? source.schemaVersion ?? 0)
  const panels = Array.isArray(source.panels) ? (source.panels as Array<Record<string, unknown>>) : []
  const activePanelId = String(source.active_panel_id ?? source.activePanelId ?? '')
  if (schemaVersion !== 1) {
    throw new ApiClientError('unsupported dashboard layout schema', 'UNSUPPORTED_SCHEMA', 400, null)
  }
  if (panels.length === 0) {
    throw new ApiClientError('dashboard layout must contain at least one panel', 'EMPTY_PANELS', 400, null)
  }
  if (panels.length > 8) {
    throw new ApiClientError('dashboard layout exceeds panel limit', 'TOO_MANY_PANELS', 400, null)
  }

  const ids = new Set<string>()
  const names = new Set<string>()
  for (const panel of panels) {
    const panelId = String(panel.id ?? '')
    const panelName = String(panel.name ?? '')
    const widgets = Array.isArray(panel.widgets) ? panel.widgets : []
    if (!panelId || ids.has(panelId)) {
      throw new ApiClientError('dashboard panel id is duplicated', 'DUPLICATE_PANEL_ID', 400, null)
    }
    if (!panelName || panelName.length > 32) {
      throw new ApiClientError('dashboard panel name is invalid', 'PANEL_NAME_TOO_LONG', 400, null)
    }
    const name = panelName.toLowerCase()
    if (names.has(name)) {
      throw new ApiClientError('dashboard panel name is duplicated', 'DUPLICATE_PANEL_NAME', 400, null)
    }
    ids.add(panelId)
    names.add(name)

    const widgetDeviceIds = new Set<number>()
    for (const widget of widgets) {
      const tuple = Array.isArray(widget) ? widget : []
      const deviceId = Number(tuple[0] ?? 0)
      const x = Number(tuple[1] ?? 0)
      const y = Number(tuple[2] ?? 0)
      const w = Number(tuple[3] ?? 0)
      const h = Number(tuple[4] ?? 0)
      if (
        deviceId <= 0 ||
        x < 0 ||
        y < 0 ||
        w <= 0 ||
        h <= 0 ||
        widgetDeviceIds.has(deviceId)
      ) {
        throw new ApiClientError('dashboard widget coordinates are invalid', 'INVALID_WIDGET', 400, null)
      }
      widgetDeviceIds.add(deviceId)
    }
  }

  if (!ids.has(activePanelId)) {
    throw new ApiClientError('dashboard active panel does not exist', 'INVALID_ACTIVE_PANEL', 400, null)
  }
}
