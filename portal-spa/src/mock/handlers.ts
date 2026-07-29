import type {
  DashboardLayoutRecord,
  DashboardLayoutWidgetRecord,
  DashboardLayoutResponse,
  DeviceCommandRequest,
  DeviceDetailResponse,
  DeviceFlushResponse,
  DeviceLayoutResponse,
  DeviceCreateRequest,
  DeviceMutationResponse,
  MetricPlaceholderCatalogResponse,
  MetricPlaceholderDescriptor,
  MetricValuesResponse,
  DeviceSetupTransferResponse,
  DeviceRegistryResponse,
  MqttSettingsRecord,
  MqttStatusResponse,
  OtaStatusResponse,
  PersistenceSettingsRecord,
  SystemRestartResponse,
  SystemStatusResponse,
  SystemVersionResponse,
  TimeSettingsRecord,
  TimeStatusResponse,
  WifiScanResponse,
  WifiStatusResponse,
} from '@/api'
import type {
  DoseJournalResponse,
  DosingPumpContainerSnapshot,
  DosingPumpOutputSnapshot,
  SchedulePresetPoint,
  SchedulePresetsResponse,
} from '@/api/contracts'
import type { BlobUploadResponse, DeviceDependencyLink, TemperatureOutputSnapshot } from '@/api/contracts'
import {
  defaultSsd1306Layout,
  normalizeSsd1306Layout,
} from '@/models/devices/ssd1306/layout'
import { ApiClientError } from '@/api/http'
import { deviceTypeIdFromName } from '@/models/device-type-ids.ts'
import { normalizeDisplayRotation } from '@/models/devices/display/orientation'
import { resolvePanelGeometry } from '@/models/devices/display/panels'
import { publishRealtimeMessage } from '@/realtime/bus'
import { scheduleMockPersistenceFlush } from '@/realtime/mockRuntime'
import {
  canonicalizeDeviceRecord,
  createSeedMockDatabase,
  createDeviceRecord,
  isHaSupportedTypeName,
  loadMockDatabase,
  nowLocalFlavoredEpochSeconds,
  saveMockDatabase,
  type MockDeviceRecord,
  type MockDoseJournalEntry,
  type MockSchedulePreset,
} from './database'
import {
  createGpioSwitchDevice,
  createOneWireBusDevice,
  createI2cBusDevice,
  createSsd1306Device,
  createSt7735Device,
  createSpiBusDevice,
  createDs18b20Device,
  createNtcThermistorDevice,
  createAht10Device,
  createDht11Device,
  createHtu21Device,
  normalizeAht10ConfigPayload,
  normalizeDht11ConfigPayload,
  normalizeRtcDs1302ConfigPayload,
  normalizeHtu21ConfigPayload,
  createThermostatDevice,
  createRtcDs3231Device,
  createRtcDs1302Device,
  createPcf8574ExpanderDevice,
  createPcf8575ExpanderDevice,
  createPortExpanderSwitchDevice,
  createDummyDevice,
  createScheduleDevice,
  createAutoSwitchDevice,
  createBinarySensorDevice,
  createDosingPumpDevice,
  createAnalogOutputDevice,
  createFadeAnalogOutputDevice,
  createScheduledAnalogOutputDevice,
  createAnalogOutputComposerDevice,
  normalizeAnalogOutputConfigPayload,
  normalizeFadeAnalogOutputConfigPayload,
  normalizeScheduledAnalogOutputConfigPayload,
  normalizeDosingPumpConfigPayload,
  requireDosingPumpDependencies,
  normalizeScheduleRule,
  requireAutoSwitchDependencies,
  autoSwitchConditionsSatisfied,
  normalizeDependencyLinks,
  dependencyDeviceIdForRole,
  normalizeFiniteNumber,
  normalizeI2cAddress,
  normalizeDisplayPanel,
  requirePortExpanderDependency,
  portExpanderChannelCount,
  ensureUniquePortExpanderChannel,
  ensureUniqueI2cAddressAcrossTypes,
  createAnalogPortInputDevice,
  createAds1115HubDevice,
  createCd74hc4067HubDevice,
  createAnalogInputChannelDevice,
} from './device-factories'

function isRecordPayload(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

type DeviceRecord = MockDeviceRecord

function validateAnalogOutputDependencies(
  db: ReturnType<typeof createSeedMockDatabase>,
  deps: DeviceDependencyLink[],
  ownerDeviceId: number,
  maxOutputs: number,
): DeviceDependencyLink[] {
  const analogDeps = deps.filter(link => link.role === 'analog_output')
  if (analogDeps.length === 0 || analogDeps.length > maxOutputs || (maxOutputs === 1 && analogDeps.length !== 1)) {
    throw new ApiClientError('analog output dependency count is invalid', 'BAD_ARGS', 400, null)
  }
  const uniqueIds = new Set(analogDeps.map(link => link.deviceId))
  if (uniqueIds.size !== analogDeps.length) {
    throw new ApiClientError('analog output dependency is duplicated', 'BAD_ARGS', 400, null)
  }
  for (const link of analogDeps) {
    const target = db.devices.find(entry => entry.record.id === link.deviceId)
    if (!target || !['analog_output', 'fade_analog_output', 'scheduled_analog_output'].includes(target.record.typeName)) {
      throw new ApiClientError('analog output dependency is invalid', 'BAD_ARGS', 400, null)
    }
    const competingOwner = db.devices.find(entry =>
      entry.record.id !== ownerDeviceId &&
      ['fade_analog_output', 'scheduled_analog_output', 'analog_output_composer'].includes(entry.record.typeName) &&
      (entry.config.deps ?? []).some(dependency =>
        dependency.role === 'analog_output' && dependency.deviceId === link.deviceId),
    )
    if (competingOwner) {
      throw new ApiClientError('analog output dependency already has a controlling owner', 'INVALID_RELATIONSHIP', 400, null)
    }
  }
  return analogDeps
}

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
  const canonical = canonicalizeDeviceRecord(device)
  // Mirror the firmware: the display layout is not embedded in the device payload; it loads via
  // GET /api/devices/:id/layout. Keep it in the DB, just omit it from the API response.
  if (canonical.config && typeof canonical.config === 'object' && 'layout' in canonical.config) {
    const { layout: _omitted, ...configWithoutLayout } = canonical.config as Record<string, unknown>
    return { ...canonical, config: configWithoutLayout as DeviceRecord['config'] }
  }
  return canonical
}

export function mockFetchDeviceLayout(deviceId: number, page?: number): DeviceLayoutResponse {
  const db = loadMockDatabase()
  const device = db.devices.find(entry => entry.record.id === deviceId)
  if (!device) {
    throw new ApiClientError('device not found', 'NOT_FOUND', 404, null)
  }
  const layout = (device.config as Record<string, unknown>).layout as
    | { schemaVersion?: number; activePageId?: string; pages?: Array<Record<string, unknown>> }
    | undefined
  if (!layout) {
    throw new ApiClientError('device has no display layout', 'NOT_FOUND', 404, null)
  }
  const allPages = Array.isArray(layout.pages) ? layout.pages : []
  const pages =
    typeof page === 'number' && page >= 0 ? (allPages[page] !== undefined ? [allPages[page]] : []) : allPages
  return ok({
    schemaVersion: layout.schemaVersion ?? 1,
    activePageId: layout.activePageId ?? 'main',
    pages,
  }) as unknown as DeviceLayoutResponse
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
    bleProvisioningSupported: db.wifi.bleProvisioningSupported,
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
  previewNumber?: number,
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
    previewNumber,
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
    if (device.record.typeName === 'htu21') {
      const output = device.runtime.output as { temperature?: TemperatureOutputSnapshot; humidity?: { value: number; unitSymbol: string; valid: boolean } } | undefined
      const temperature = output?.temperature
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
      const humidity = output?.humidity
      placeholders.push(metricDescriptor(
        'dev',
        device.record.id,
        label,
        101,
        'humidity',
        `${label} humidity`,
        'float',
        Boolean(humidity?.valid),
        humidity?.valid ? `${humidity.value.toFixed(1)} ${humidity.unitSymbol}` : '',
      ))
    }
    if (device.record.typeName === 'gpio_switch') {
      const state = (device.runtime.output as { state?: boolean } | undefined)?.state
      placeholders.push(
        metricDescriptor(
          'dev',
          device.record.id,
          label,
          200,
          'state',
          `${label} state`,
          'string',
          typeof state === 'boolean',
          typeof state === 'boolean' ? (state ? 'on' : 'off') : '',
        ),
      )
    }
    if (device.record.typeName === 'aht10' || device.record.typeName === 'dht11') {
      const output = device.runtime.output as { temperature?: TemperatureOutputSnapshot; humidity?: { value: number; unitSymbol: string; valid: boolean } } | undefined
      const temperature = output?.temperature
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
      const humidity = output?.humidity
      placeholders.push(metricDescriptor(
        'dev',
        device.record.id,
        label,
        101,
        'humidity',
        `${label} humidity`,
        'float',
        Boolean(humidity?.valid),
        humidity?.valid ? `${humidity.value.toFixed(1)} ${humidity.unitSymbol}` : '',
      ))
    }
    if (device.record.typeName === 'analog_output') {
      const state = (device.runtime.output as { state?: number } | undefined)?.state
      placeholders.push(metricDescriptor(
        'dev',
        device.record.id,
        label,
        300,
        'state_percent',
        `${label} state`,
        'float',
        typeof state === 'number',
        typeof state === 'number' ? `${Math.round(state)}%` : '',
        typeof state === 'number' ? state : undefined,
      ))
    }
  }
  // Fixed mock instants so `format`/`fixed` filter previews are deterministic in the designer.
  const mockLocalEpochSeconds = Math.floor(Date.UTC(2026, 0, 15, 10, 30, 0) / 1000)
  const mockUptimeMs = 3723000
  placeholders.push(metricDescriptor('system', 0, undefined, 1, 'time', 'System time', 'datetime', true, '10:30:00', mockLocalEpochSeconds))
  placeholders.push(metricDescriptor('system', 0, undefined, 2, 'uptime', 'System uptime', 'duration', true, '1:02:03', mockUptimeMs))
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
      typeName !== 'spi_bus' &&
      typeName !== 'ssd1306' &&
      typeName !== 'st7735' &&
      typeName !== 'ds18b20_temperature_sensor' &&
      typeName !== 'ntc_thermistor_temperature_sensor' &&
      typeName !== 'aht10' &&
      typeName !== 'dht11' &&
      typeName !== 'htu21' &&
      typeName !== 'thermostat' &&
      typeName !== 'rtc_ds3231' &&
      typeName !== 'rtc_ds1302' &&
      typeName !== 'pcf8574_expander' &&
      typeName !== 'pcf8575_expander' &&
      typeName !== 'port_expander_switch' &&
      typeName !== 'schedule' &&
      typeName !== 'auto_switch' &&
      typeName !== 'binary_sensor' &&
      typeName !== 'dosing_pump' &&
      typeName !== 'analog_output' &&
      typeName !== 'fade_analog_output' &&
      typeName !== 'scheduled_analog_output' &&
      typeName !== 'analog_output_composer' &&
      typeName !== 'analog_port_input' &&
      typeName !== 'ads1115_hub' &&
      typeName !== 'cd74hc4067_hub' &&
      typeName !== 'analog_input_channel'
    ) {
      throw new ApiClientError('unsupported device type', 'UNSUPPORTED_TYPE', 400, null)
    }

    const configSource = isRecordPayload(rawPayload.config) ? rawPayload.config : {}
    const baseDeps = Array.isArray(configSource.deps) ? normalizeDependencyLinks(configSource.deps) : []
    const enabled = typeof configSource.enabled === 'boolean' ? configSource.enabled : true
    const name = typeof configSource.name === 'string' && configSource.name.length > 0 ? configSource.name : 'New Device'
    if (typeName === 'fade_analog_output' || typeName === 'scheduled_analog_output') {
      validateAnalogOutputDependencies(db, baseDeps, nextId, 1)
    } else if (typeName === 'analog_output_composer') {
      validateAnalogOutputDependencies(db, baseDeps, nextId, 16)
    }

    let device: DeviceRecord = (() => {
      switch (typeName) {
        case 'gpio_switch':
          return createGpioSwitchDevice(nextId, configSource, baseDeps, enabled, name)
        case 'onewire_bus':
          return createOneWireBusDevice(nextId, configSource, baseDeps, enabled, name)
        case 'i2c_bus':
          return createI2cBusDevice(nextId, configSource, baseDeps, enabled, name)
        case 'spi_bus':
          return createSpiBusDevice(nextId, configSource, baseDeps, enabled, name)
        case 'ssd1306':
          return createSsd1306Device(nextId, configSource, baseDeps, enabled, name, db)
        case 'st7735':
          return createSt7735Device(nextId, configSource, baseDeps, enabled, name, db)
        case 'ds18b20_temperature_sensor':
          return createDs18b20Device(nextId, configSource, baseDeps, enabled, name, db)
        case 'ntc_thermistor_temperature_sensor':
          return createNtcThermistorDevice(nextId, configSource, baseDeps, enabled, name, db)
        case 'aht10':
          return createAht10Device(nextId, configSource, baseDeps, enabled, name, db)
        case 'dht11':
          return createDht11Device(nextId, configSource, baseDeps, enabled, name, db)
        case 'htu21':
          return createHtu21Device(nextId, configSource, baseDeps, enabled, name, db)
        case 'thermostat':
          return createThermostatDevice(nextId, configSource, baseDeps, enabled, name, db)
        case 'rtc_ds3231':
          return createRtcDs3231Device(nextId, configSource, baseDeps, enabled, name, db)
        case 'rtc_ds1302':
          return createRtcDs1302Device(nextId, configSource, baseDeps, enabled, name, db)
        case 'pcf8574_expander':
          return createPcf8574ExpanderDevice(nextId, configSource, baseDeps, enabled, name, db)
        case 'pcf8575_expander':
          return createPcf8575ExpanderDevice(nextId, configSource, baseDeps, enabled, name, db)
        case 'port_expander_switch':
          return createPortExpanderSwitchDevice(nextId, configSource, baseDeps, enabled, name, db)
        case 'schedule':
          return createScheduleDevice(nextId, configSource, baseDeps, enabled, name)
        case 'auto_switch':
          return createAutoSwitchDevice(nextId, configSource, baseDeps, enabled, name, db)
        case 'binary_sensor':
          return createBinarySensorDevice(nextId, configSource, baseDeps, enabled, name)
        case 'dosing_pump':
          return createDosingPumpDevice(nextId, configSource, baseDeps, enabled, name, db)
        case 'analog_output':
          return createAnalogOutputDevice(nextId, configSource, baseDeps, enabled, name)
        case 'fade_analog_output':
          return createFadeAnalogOutputDevice(nextId, configSource, baseDeps, enabled, name)
        case 'scheduled_analog_output':
          return createScheduledAnalogOutputDevice(nextId, configSource, baseDeps, enabled, name)
        case 'analog_output_composer':
          return createAnalogOutputComposerDevice(nextId, configSource, baseDeps, enabled, name, db)
        case 'analog_port_input':
          return createAnalogPortInputDevice(nextId, configSource, baseDeps, enabled, name)
        case 'ads1115_hub':
          return createAds1115HubDevice(nextId, configSource, baseDeps, enabled, name, db)
        case 'cd74hc4067_hub':
          return createCd74hc4067HubDevice(nextId, configSource, baseDeps, enabled, name)
        case 'analog_input_channel':
          return createAnalogInputChannelDevice(nextId, configSource, baseDeps, enabled, name, db)
        default:
          return createDummyDevice(nextId, configSource, baseDeps, enabled, name)
      }
    })()

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
      case 'delete':
        removedDevice = { ...device }
        db.devices = db.devices.filter(entry => entry.record.id !== deviceId)
        pruneDashboardLayout(db)
        break
      case 'updateConfig':
      case 'update_config': {
        if (!isRecordPayload(payload.config)) {
          throw new ApiClientError('config is required', 'BAD_ARGS', 400, null)
        }
        if (typeof payload.config.name === 'string' && payload.config.name.trim().length > 0) {
          const nextName = payload.config.name.trim()
          if (nextName !== device.config.name
            && db.devices.some(entry => entry.record.id !== deviceId && entry.config.name === nextName)) {
            throw new ApiClientError('device name already exists', 'INVALID_CONFIG', 400, null)
          }
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
            startupState: typeof payload.config.startupState === 'boolean' ? payload.config.startupState : false,
            safeState: typeof payload.config.safeState === 'boolean' ? payload.config.safeState : false,
            inverted: Boolean(payload.config.inverted ?? false),
            gpioPin: normalizeFiniteNumber(payload.config.gpioPin, normalizeFiniteNumber(currentConfig['gpioPin'], 2)),
          }
          const status = device.config.enabled ? 'ready' : 'disabled'
          device.runtime.lifecycleStatus = status
          device.runtime.effectiveStatus = status
          device.runtime.status = status
        } else if (device.record.typeName === 'analog_output') {
          const config = normalizeAnalogOutputConfigPayload({
            ...device.config,
            ...payload.config,
          }, device.config.enabled)
          device.config = {
            ...config,
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: device.config.deps,
          }
        } else if (device.record.typeName === 'fade_analog_output') {
          const dependencyLinks = validateAnalogOutputDependencies(
            db,
            normalizeDependencyLinks(payload.deps ?? device.config.deps),
            device.record.id,
            1,
          )
          const config = normalizeFadeAnalogOutputConfigPayload({
            ...device.config,
            ...payload.config,
          }, device.config.enabled)
          device.config = {
            ...device.config,
            ...config,
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
          }
        } else if (device.record.typeName === 'scheduled_analog_output') {
          const dependencyLinks = validateAnalogOutputDependencies(
            db,
            normalizeDependencyLinks(payload.deps ?? device.config.deps),
            device.record.id,
            1,
          )
          const config = normalizeScheduledAnalogOutputConfigPayload({
            ...device.config,
            ...payload.config,
          }, device.config.enabled)
          device.config = {
            ...device.config,
            ...config,
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
          }
        } else if (device.record.typeName === 'analog_output_composer') {
          const dependencyLinks = validateAnalogOutputDependencies(
            db,
            normalizeDependencyLinks(payload.deps ?? device.config.deps),
            device.record.id,
            16,
          )
          device.config = {
            ...device.config,
            enabled: typeof payload.config.enabled === 'boolean' ? payload.config.enabled : device.config.enabled,
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
          }
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
          const i2cAddress = normalizeI2cAddress(payload.config.i2cAddress, normalizeFiniteNumber(currentConfig['i2cAddress'], 60))
          ensureUniqueI2cAddress(db, dependencyDeviceId, i2cAddress, device.record.id)
          const ssd1306Panel = normalizeDisplayPanel('ssd1306', payload.config.panel, String(currentConfig['panel'] ?? '128x64'))
          const ssd1306Geometry = resolvePanelGeometry('ssd1306', ssd1306Panel)
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
            i2cAddress,
            rotation: normalizeDisplayRotation(payload.config.rotation, normalizeFiniteNumber(currentConfig['rotation'], 0)),
            panel: ssd1306Panel,
            width: ssd1306Geometry?.width ?? normalizeFiniteNumber(payload.config.width, normalizeFiniteNumber(currentConfig['width'], 128)),
            height: ssd1306Geometry?.height ?? normalizeFiniteNumber(payload.config.height, normalizeFiniteNumber(currentConfig['height'], 64)),
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
          const st7735Panel = normalizeDisplayPanel('st7735', payload.config.panel, String(currentConfig['panel'] ?? 'black18'))
          const st7735Geometry = resolvePanelGeometry('st7735', st7735Panel)
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
            rotation: normalizeDisplayRotation(payload.config.rotation, normalizeFiniteNumber(currentConfig['rotation'], 0)),
            panel: st7735Panel,
            width: st7735Geometry?.width ?? 128,
            height: st7735Geometry?.height ?? 160,
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
        } else if (device.record.typeName === 'spi_bus') {
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: device.config.deps,
            host: normalizeFiniteNumber(payload.config.host, normalizeFiniteNumber(currentConfig['host'], 2)),
            sckPin: normalizeFiniteNumber(payload.config.sckPin, normalizeFiniteNumber(currentConfig['sckPin'], 18)),
            mosiPin: normalizeFiniteNumber(payload.config.mosiPin, normalizeFiniteNumber(currentConfig['mosiPin'], 23)),
            misoPin: normalizeFiniteNumber(payload.config.misoPin, normalizeFiniteNumber(currentConfig['misoPin'], -1)),
          }
          device.runtime.lifecycleStatus = 'ready'
          device.runtime.effectiveStatus = 'ready'
          device.runtime.status = 'ready'
          device.runtime.transactionActive = false
          device.runtime.generation = normalizeFiniteNumber(device.runtime.generation, 0) + 1
        } else if (device.record.typeName === 'rtc_ds3231') {
          const dependencyLinks = normalizeDependencyLinks(payload.deps ?? device.config.deps)
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'i2c_bus')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError('rtc_ds3231 i2c dependency is required', 'BAD_ARGS', 400, null)
          }
          requireI2cDependency(db, dependencyDeviceId)
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          const i2cAddress = normalizeI2cAddress(
            payload.config.i2cAddress,
            normalizeFiniteNumber(currentConfig['i2cAddress'], 0x68),
          )
          ensureUniqueI2cAddressAcrossTypes(db, dependencyDeviceId, i2cAddress, device.record.id)
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
            i2cAddress,
            useForSystemTimeSync: Boolean(payload.config.useForSystemTimeSync ?? currentConfig['useForSystemTimeSync']),
          }
        } else if (device.record.typeName === 'htu21') {
          const dependencyLinks = normalizeDependencyLinks(payload.deps ?? device.config.deps)
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'i2c_bus')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError('htu21 i2c dependency is required', 'BAD_ARGS', 400, null)
          }
          requireI2cDependency(db, dependencyDeviceId)
          const config = normalizeHtu21ConfigPayload({
            ...device.config,
            ...payload.config,
          }, device.config.enabled)
          ensureUniqueI2cAddressAcrossTypes(db, dependencyDeviceId, config.i2cAddress as number, device.record.id)
          device.config = {
            ...config,
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
          }
        } else if (device.record.typeName === 'aht10') {
          const dependencyLinks = normalizeDependencyLinks(payload.deps ?? device.config.deps)
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'i2c_bus')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError('aht10 i2c dependency is required', 'BAD_ARGS', 400, null)
          }
          requireI2cDependency(db, dependencyDeviceId)
          const config = normalizeAht10ConfigPayload({
            ...device.config,
            ...payload.config,
          }, device.config.enabled)
          ensureUniqueI2cAddressAcrossTypes(db, dependencyDeviceId, config.i2cAddress as number, device.record.id)
          device.config = {
            ...config,
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
          }
        } else if (device.record.typeName === 'dht11') {
          if (Array.isArray(payload.deps) && payload.deps.length > 0) {
            throw new ApiClientError('dht11 does not use dependencies', 'BAD_ARGS', 400, null)
          }
          device.config = {
            ...normalizeDht11ConfigPayload({
              ...device.config,
              ...payload.config,
            }, device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: [],
          }
        } else if (device.record.typeName === 'rtc_ds1302') {
          if (Array.isArray(payload.deps) && payload.deps.length > 0) {
            throw new ApiClientError('rtc_ds1302 does not use dependencies', 'BAD_ARGS', 400, null)
          }
          device.config = {
            ...normalizeRtcDs1302ConfigPayload({
              ...device.config,
              ...payload.config,
            }, device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: [],
          }
        } else if (device.record.typeName === 'pcf8574_expander' || device.record.typeName === 'pcf8575_expander') {
          const dependencyLinks = normalizeDependencyLinks(payload.deps ?? device.config.deps)
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'i2c_bus')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError(`${device.record.typeName} i2c dependency is required`, 'BAD_ARGS', 400, null)
          }
          requireI2cDependency(db, dependencyDeviceId)
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          const i2cAddress = normalizeI2cAddress(
            payload.config.i2cAddress,
            normalizeFiniteNumber(currentConfig['i2cAddress'], 0x20),
          )
          ensureUniqueI2cAddressAcrossTypes(db, dependencyDeviceId, i2cAddress, device.record.id)
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
            i2cAddress,
            inverted: Boolean(payload.config.inverted ?? currentConfig['inverted']),
          }
        } else if (device.record.typeName === 'port_expander_switch') {
          const dependencyLinks = normalizeDependencyLinks(payload.deps ?? device.config.deps)
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'port_expander')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError('port expander switch requires a port expander dependency', 'BAD_ARGS', 400, null)
          }
          const dependency = requirePortExpanderDependency(db, dependencyDeviceId)
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          const channel = Math.max(0, Math.round(normalizeFiniteNumber(payload.config.channel, normalizeFiniteNumber(currentConfig['channel'], 0))))
          if (channel >= portExpanderChannelCount(dependency)) {
            throw new ApiClientError('port expander switch channel is out of range', 'BAD_ARGS', 400, null)
          }
          ensureUniquePortExpanderChannel(db, dependencyDeviceId, channel, device.record.id)
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
            restorePreviousState: Boolean(payload.config.restorePreviousState ?? false),
            startupState: typeof payload.config.startupState === 'boolean' ? payload.config.startupState : false,
            safeState: typeof payload.config.safeState === 'boolean' ? payload.config.safeState : false,
            inverted: Boolean(payload.config.inverted ?? false),
            channel,
          }
        } else if (device.record.typeName === 'schedule') {
          const rules = Array.isArray(payload.config.rules)
            ? payload.config.rules.slice(0, 4).map(normalizeScheduleRule)
            : device.config.rules
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: device.config.deps,
            rules,
          }
        } else if (device.record.typeName === 'auto_switch') {
          const dependencyLinks = normalizeDependencyLinks(payload.deps ?? device.config.deps)
          requireAutoSwitchDependencies(db, dependencyLinks)
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
            pauseDurationSeconds: Math.max(1, normalizeFiniteNumber(payload.config.pauseDurationSeconds, normalizeFiniteNumber(currentConfig['pauseDurationSeconds'], 3600))),
          }
        } else if (device.record.typeName === 'binary_sensor') {
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          const gpioPin = Math.min(39, Math.max(0, Math.round(normalizeFiniteNumber(payload.config.gpioPin, normalizeFiniteNumber(currentConfig['gpioPin'], 4)))))
          const requestedPullMode = typeof payload.config.pullMode === 'string' ? payload.config.pullMode : currentConfig['pullMode']
          const pullMode = requestedPullMode === 'none' || requestedPullMode === 'pulldown' ? requestedPullMode : 'pullup'
          if (gpioPin >= 34 && pullMode !== 'none') {
            throw new ApiClientError('binary sensor pin has no internal pull resistors', 'INVALID_CONFIG', 400, null)
          }
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: device.config.deps,
            gpioPin,
            pullMode,
            inverted: Boolean(payload.config.inverted ?? currentConfig['inverted'] ?? false),
            debounceMs: Math.min(60000, Math.max(0, Math.round(normalizeFiniteNumber(payload.config.debounceMs, normalizeFiniteNumber(currentConfig['debounceMs'], 50))))),
          }
        } else if (device.record.typeName === 'dosing_pump') {
          const dependencyLinks = normalizeDependencyLinks(payload.deps ?? device.config.deps)
          requireDosingPumpDependencies(db, dependencyLinks)
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          const normalized = normalizeDosingPumpConfigPayload({ ...currentConfig, ...payload.config }, currentConfig)
          device.config = {
            ...device.config,
            enabled: Boolean(payload.config.enabled ?? device.config.enabled),
            name: typeof payload.config.name === 'string' && payload.config.name.length > 0
              ? payload.config.name
              : device.config.name,
            deps: dependencyLinks,
            ...normalized,
          }
          refreshMockDosingDerivedOutput(device, dependencyLinks)
        }
        if (typeof device.config.enabled === 'boolean') {
          const nextStatus = device.config.enabled ? 'ready' : 'disabled'
          device.runtime.lifecycleStatus = nextStatus
          device.runtime.effectiveStatus = nextStatus
          device.runtime.status = nextStatus
        }
        device.record.configRevision += 1
        break
      }
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
        } else if (device.record.typeName === 'rtc_ds3231' || device.record.typeName === 'pcf8574_expander' || device.record.typeName === 'pcf8575_expander' || device.record.typeName === 'htu21' || device.record.typeName === 'aht10' || device.record.typeName === 'dht11') {
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'i2c_bus')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError(`${device.record.typeName} i2c dependency is required`, 'BAD_ARGS', 400, null)
          }
          requireI2cDependency(db, dependencyDeviceId)
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          ensureUniqueI2cAddressAcrossTypes(
            db,
            dependencyDeviceId,
            normalizeFiniteNumber(
              currentConfig['i2cAddress'],
              device.record.typeName === 'rtc_ds3231'
                ? 0x68
                : device.record.typeName === 'htu21'
                  ? 0x40
                  : device.record.typeName === 'aht10'
                    ? 0x38
                    : device.record.typeName === 'dht11'
                      ? 0x17
                      : 0x20,
            ),
            device.record.id,
          )
          device.config = {
            ...device.config,
            deps: dependencyLinks,
          }
        } else if (device.record.typeName === 'port_expander_switch') {
          const dependencyDeviceId = dependencyDeviceIdForRole(dependencyLinks, 'port_expander')
          if (dependencyDeviceId <= 0) {
            throw new ApiClientError('port expander switch requires a port expander dependency', 'BAD_ARGS', 400, null)
          }
          const dependency = requirePortExpanderDependency(db, dependencyDeviceId)
          const currentConfig = (isRecordPayload(device.config) ? device.config : {}) as Record<string, unknown>
          const channel = normalizeFiniteNumber(currentConfig['channel'], 0)
          if (channel >= portExpanderChannelCount(dependency)) {
            throw new ApiClientError('port expander switch channel is out of range', 'BAD_ARGS', 400, null)
          }
          ensureUniquePortExpanderChannel(db, dependencyDeviceId, channel, device.record.id)
          device.config = {
            ...device.config,
            deps: dependencyLinks,
          }
        } else if (
          device.record.typeName === 'fade_analog_output' ||
          device.record.typeName === 'scheduled_analog_output' ||
          device.record.typeName === 'analog_output_composer'
        ) {
          const analogLinks = validateAnalogOutputDependencies(
            db,
            dependencyLinks,
            device.record.id,
            device.record.typeName === 'analog_output_composer' ? 16 : 1,
          )
          device.config = { ...device.config, deps: analogLinks }
        } else {
          device.config = {
            ...device.config,
            deps: dependencyLinks,
          }
        }
        device.record.configRevision += 1
        break
      }
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
      case 'checkDevice':
        if (device.record.typeName === 'spi_bus') {
          const csPin = typeof payload.csPin === 'number' ? payload.csPin : 0
          const outcomes: Array<'detected' | 'not_detected' | 'inconclusive'> = ['detected', 'not_detected', 'inconclusive']
          const methods: Array<'miso_activity' | 'cs_pull_heuristic'> = ['miso_activity', 'cs_pull_heuristic']
          const randomOutcome = outcomes[Math.floor(Math.random() * outcomes.length)]
          const randomMethod = methods[Math.floor(Math.random() * methods.length)]
          device.runtime.probe = {
            ready: true,
            csPin,
            outcome: randomOutcome,
            method: randomMethod,
            checkedAtMs: Date.now(),
          }
          break
        }
        throw new ApiClientError('checkDevice only supported on spi_bus', 'BAD_ARGS', 400, null)
      case 'setOutput':
      case 'set_output': {
        if (!device.config.enabled) {
          throw new ApiClientError('device is disabled', 'INVALID_STATE', 409, null)
        }
        if (
          device.record.typeName === 'analog_output' ||
          device.record.typeName === 'fade_analog_output' ||
          device.record.typeName === 'scheduled_analog_output'
        ) {
          const currentOutput = (isRecordPayload(device.runtime.output) ? device.runtime.output : {}) as Record<string, unknown>
          if (typeof payload.state !== 'number' || !Number.isFinite(payload.state)) {
            throw new ApiClientError('state must be numeric', 'BAD_ARGS', 400, null)
          }
          const state = Math.min(100, Math.max(0, Math.round(payload.state)))
          device.runtime.output = {
            ...currentOutput,
            state,
            ...(device.record.typeName === 'fade_analog_output'
              ? { targetState: state, transitioning: false }
              : {}),
            ...(device.record.typeName === 'scheduled_analog_output'
              ? { requestedState: state, mode: 'manual' }
              : {}),
          }
          break
        }
        if (typeof payload.state !== 'boolean') {
          throw new ApiClientError('state must be boolean', 'BAD_ARGS', 400, null)
        }
        if (device.record.typeName === 'gpio_switch') {
          device.runtime.output = {
            state: payload.state,
          }
        } else if (device.record.typeName === 'port_expander_switch') {
          device.runtime.output = {
            state: payload.state,
          }
          // Mirrors the firmware's IPortExpanderRuntime::requestChannelState() writing straight
          // through to the expander's in-memory bitmask - keeps the expander's channel dots (and
          // any other switch depending on the same expander) visually consistent in mock mode.
          const dependencyDeviceId = dependencyDeviceIdForRole((device.config.deps ?? []) as DeviceDependencyLink[], 'port_expander')
          const dependency = db.devices.find(entry => entry.record.id === dependencyDeviceId)
          if (dependency) {
            const channel = normalizeFiniteNumber((device.config as Record<string, unknown>).channel, 0)
            const currentStates = normalizeFiniteNumber((dependency.runtime.output as Record<string, unknown> | undefined)?.channelStates, 0)
            const nextStates = payload.state
              ? currentStates | (1 << channel)
              : currentStates & ~(1 << channel)
            dependency.runtime.output = {
              ...(isRecordPayload(dependency.runtime.output) ? dependency.runtime.output : {}),
              channelStates: nextStates,
            }
          }
        } else if (device.record.typeName === 'auto_switch') {
          // Mirrors AutoSwitchDevice::requestOutputState(): a plain on/off is a manual override -
          // it sets mode directly, exiting Paused the same way any other setMode() call does.
          device.runtime.output = {
            ...(isRecordPayload(device.runtime.output) ? device.runtime.output : {}),
            mode: payload.state ? 'on' : 'off',
            paused: false,
            pausedUntilMs: 0,
            state: payload.state,
          }
        } else {
          throw new ApiClientError('unsupported output command', 'BAD_ARGS', 400, null)
        }
        break
      }
      case 'setMode': {
        if (device.record.typeName === 'dosing_pump') {
          const modeValue = typeof payload.mode === 'string' ? payload.mode : ''
          if (modeValue !== 'auto' && modeValue !== 'manual') {
            throw new ApiClientError('unsupported mode value', 'BAD_ARGS', 400, null)
          }
          const currentOutput = (isRecordPayload(device.runtime.output) ? device.runtime.output : {}) as Record<string, unknown>
          device.runtime.output = { ...currentOutput, autoMode: modeValue === 'auto' }
          break
        }
        if (device.record.typeName === 'scheduled_analog_output') {
          const modeValue = typeof payload.mode === 'string' ? payload.mode : ''
          if (modeValue !== 'off' && modeValue !== 'manual' && modeValue !== 'scheduled') {
            throw new ApiClientError('unsupported analog output mode', 'BAD_ARGS', 400, null)
          }
          const currentOutput = isRecordPayload(device.runtime.output as unknown) ? device.runtime.output as unknown as Record<string, unknown> : {}
          device.runtime.output = {
            ...currentOutput,
            mode: modeValue,
            ...(modeValue === 'off' ? { state: 0, requestedState: 0 } : {}),
          }
          break
        }
        if (device.record.typeName === 'analog_output_composer') {
          const modeValue = typeof payload.mode === 'string' ? payload.mode : ''
          if (modeValue !== 'off' && modeValue !== 'manual' && modeValue !== 'scheduled') {
            throw new ApiClientError('unsupported analog output composer mode', 'BAD_ARGS', 400, null)
          }
          const currentOutput = isRecordPayload(device.runtime.output as unknown) ? device.runtime.output as unknown as Record<string, unknown> : {}
          const dependencies = Array.isArray(device.config.deps) ? device.config.deps as DeviceDependencyLink[] : []
          for (const dependency of dependencies.filter(link => link.role === 'analog_output')) {
            const target = db.devices.find(entry => entry.record.id === dependency.deviceId)
            if (target) {
              target.runtime.output = {
                ...(isRecordPayload(target.runtime.output) ? target.runtime.output : {}),
                ...(target.record.typeName === 'scheduled_analog_output' ? { mode: modeValue } : {}),
                ...(modeValue === 'off' ? { state: 0, requestedState: 0 } : {}),
              }
            }
          }
          device.runtime.output = { ...currentOutput, mode: modeValue }
          break
        }
        if (device.record.typeName !== 'auto_switch') {
          throw new ApiClientError('setMode is not supported on this device', 'BAD_ARGS', 400, null)
        }
        const modeValue = typeof payload.mode === 'string' ? payload.mode : ''
        const currentOutput = (isRecordPayload(device.runtime.output) ? device.runtime.output : {}) as Record<string, unknown>
        // Mirrors AutoSwitchDevice's flat AutoSwitchMode (Off/On/Auto/Paused, not Paused as a
        // separate overlay on Auto) - there is no "resume" command, "auto" is the only entry point
        // into Auto from anywhere including Paused, mirroring handleCommand()'s "auto" branch.
        if (modeValue === 'auto') {
          const deps = (device.config.deps ?? []) as DeviceDependencyLink[]
          const state = autoSwitchConditionsSatisfied(db, deps)
          device.runtime.output = { ...currentOutput, mode: 'auto', paused: false, pausedUntilMs: 0, state }
        } else if (modeValue === 'pause') {
          // Mirrors handleCommand()'s "pause" branch: only takes effect from Auto.
          if (currentOutput.mode !== 'auto') {
            throw new ApiClientError('pause is only available from auto mode', 'BAD_ARGS', 400, null)
          }
          const pauseDurationSeconds = normalizeFiniteNumber((device.config as Record<string, unknown>).pauseDurationSeconds, 3600)
          device.runtime.output = {
            ...currentOutput,
            mode: 'paused',
            paused: true,
            pausedUntilMs: Date.now() + pauseDurationSeconds * 1000,
            state: false,
          }
        } else {
          throw new ApiClientError('unsupported mode value', 'BAD_ARGS', 400, null)
        }
        break
      }
      case 'startDose': {
        if (device.record.typeName !== 'dosing_pump') {
          throw new ApiClientError('startDose only supported on dosing_pump', 'BAD_ARGS', 400, null)
        }
        const amountMl = normalizeFiniteNumber(payload.amountMl, 0)
        if (!(amountMl > 0) || amountMl > 655.35) {
          throw new ApiClientError('amountMl is required and must be 0.01-655.35', 'BAD_ARGS', 400, null)
        }
        startMockDosingRun(device, amountMl, payload.logging !== false)
        break
      }
      case 'stopDose': {
        if (device.record.typeName !== 'dosing_pump') {
          throw new ApiClientError('stopDose only supported on dosing_pump', 'BAD_ARGS', 400, null)
        }
        finishMockDosingRun(db, device, Date.now())
        break
      }
      case 'setVolume': {
        if (device.record.typeName !== 'dosing_pump') {
          throw new ApiClientError('setVolume only supported on dosing_pump', 'BAD_ARGS', 400, null)
        }
        const volumeMl = normalizeFiniteNumber(payload.volumeMl, -1)
        if (volumeMl < 0) {
          throw new ApiClientError('volumeMl is required and must be non-negative', 'BAD_ARGS', 400, null)
        }
        setMockDosingContainerVolume(device, volumeMl)
        break
      }
      case 'skipNext': {
        if (device.record.typeName !== 'dosing_pump') {
          throw new ApiClientError('skipNext only supported on dosing_pump', 'BAD_ARGS', 400, null)
        }
        const doseIndex = Math.round(normalizeFiniteNumber(payload.doseIndex, -1))
        const schedule = isRecordPayload(device.config.schedule) ? device.config.schedule : {}
        const doses = Array.isArray(schedule.doses) ? schedule.doses : []
        if (doseIndex < 0 || doseIndex >= doses.length) {
          throw new ApiClientError('doseIndex is required and must address a schedule dose', 'BAD_ARGS', 400, null)
        }
        const currentOutput = (isRecordPayload(device.runtime.output) ? device.runtime.output : {}) as Record<string, unknown>
        const skipNext = Array.from({ length: doses.length }, (_, index) =>
          Array.isArray(currentOutput.skipNext) ? currentOutput.skipNext[index] === true : false)
        skipNext[doseIndex] = payload.skip !== false
        device.runtime.output = { ...currentOutput, skipNext }
        break
      }
      case 'setHaSettings': {
        const haEnabled = Boolean(payload.haEnabled)
        const haName = typeof payload.haName === 'string' ? payload.haName : ''
        if (haEnabled && !isHaSupportedTypeName(device.record.typeName)) {
          throw new ApiClientError('device type does not support Home Assistant integration', 'BAD_ARGS', 400, null)
        }
        device.ha = {
          supported: isHaSupportedTypeName(device.record.typeName),
          enabled: haEnabled,
          name: haName,
          effectiveName: haName.length > 0 ? haName : device.config.name,
        }
        break
      }
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
    // Mirrors PortalWebSocketMessages::buildDeviceRemove() on the firmware: identity + removal
    // metadata only, not a full DeviceRecord - the real device is already gone by the time this fires.
    publishRealtimeMessage({
      topic: 'device.remove',
      revision: db.registryRevision,
      payload: {
        eventKind: 'device_deleted',
        deviceId,
        typeId: removedDevice ? deviceTypeIdFromName(removedDevice.record.typeName) : 0,
        registryRevision: db.registryRevision,
        name: removedDevice?.config.name ?? '',
        typeName: removedDevice?.record.typeName ?? '',
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

export function mockFetchSystemVersion(): SystemVersionResponse {
  return ok({
    version: 'mock-1.0.0',
    buildDate: new Date().toISOString(),
  })
}

export function mockFetchSystemStatus(): SystemStatusResponse {
  const db = loadMockDatabase()
  return ok(db.systemStatus)
}

function mqttConnectedFrom(mqtt: { enabled: boolean; host: string }): boolean {
  return mqtt.enabled && mqtt.host.trim().length > 0
}

function publishMqttStatus(db: ReturnType<typeof createSeedMockDatabase>): void {
  publishRealtimeMessage({
    topic: 'mqtt.status',
    revision: db.registryRevision,
    payload: {
      enabled: db.mqtt.compiledIn,
      connected: db.mqtt.connected,
      waitingForStation: db.mqtt.waitingForStation,
    },
  })
}

export function mockFetchMqttStatus(): MqttStatusResponse {
  const db = loadMockDatabase()
  return ok({
    enabled: db.mqtt.compiledIn,
    connected: db.mqtt.connected,
    waitingForStation: db.mqtt.waitingForStation,
    host: db.mqtt.host,
    port: db.mqtt.port,
    useTls: db.mqtt.useTls,
    clientId: db.mqtt.clientId,
    hasCaCert: db.mqtt.hasCaCert,
  })
}

export function mockFetchMqttSettings(): MqttSettingsRecord {
  const db = loadMockDatabase()
  return ok({
    enabled: db.mqtt.enabled,
    host: db.mqtt.host,
    port: db.mqtt.port,
    useTls: db.mqtt.useTls,
    clientId: db.mqtt.clientId,
    username: db.mqtt.username,
    password: '',
    passwordRedacted: db.mqtt.password.length > 0,
    haDiscoveryPrefix: db.mqtt.haDiscoveryPrefix,
    haNodeId: db.mqtt.haNodeId,
    haNodeName: db.mqtt.haNodeName,
    hasCaCert: db.mqtt.hasCaCert,
  })
}

export function mockUpdateMqttSettings(settings: Partial<MqttSettingsRecord>): Promise<MqttSettingsRecord> {
  const response = mutateRegistry(db => {
    db.mqtt = {
      ...db.mqtt,
      ...(settings.enabled !== undefined ? { enabled: settings.enabled } : {}),
      ...(settings.host !== undefined ? { host: settings.host } : {}),
      ...(settings.port !== undefined ? { port: settings.port } : {}),
      ...(settings.useTls !== undefined ? { useTls: settings.useTls } : {}),
      ...(settings.clientId !== undefined ? { clientId: settings.clientId } : {}),
      ...(settings.username !== undefined ? { username: settings.username } : {}),
      ...(settings.password !== undefined ? { password: settings.password } : {}),
      ...(settings.haDiscoveryPrefix !== undefined ? { haDiscoveryPrefix: settings.haDiscoveryPrefix } : {}),
      ...(settings.haNodeId !== undefined ? { haNodeId: settings.haNodeId } : {}),
      ...(settings.haNodeName !== undefined ? { haNodeName: settings.haNodeName } : {}),
    }
    db.mqtt.connected = mqttConnectedFrom(db.mqtt)
    db.mqtt.waitingForStation = db.mqtt.enabled && !db.mqtt.connected
    return mockFetchMqttSettings()
  })
  publishMqttStatus(loadMockDatabase())
  return Promise.resolve(response)
}

export function mockUploadMqttCaCert(_file: File): Promise<MqttStatusResponse> {
  mutateRegistry(db => {
    db.mqtt.hasCaCert = true
    return null
  })
  return Promise.resolve(mockFetchMqttStatus())
}

export function mockDeleteMqttCaCert(): Promise<MqttStatusResponse> {
  mutateRegistry(db => {
    db.mqtt.hasCaCert = false
    return null
  })
  return Promise.resolve(mockFetchMqttStatus())
}

// Generic blob store mock (docs/blob-store.md) - backed by MockDatabase.blobs (key -> base64), the
// same shape/convention the real backend uses.

function arrayBufferToBase64(buffer: ArrayBuffer): string {
  const bytes = new Uint8Array(buffer)
  let binary = ''
  for (let index = 0; index < bytes.length; index++) {
    binary += String.fromCharCode(bytes[index])
  }
  return globalThis.btoa(binary)
}

function base64ToBlob(base64: string): Blob {
  const binary = globalThis.atob(base64)
  const bytes = new Uint8Array(binary.length)
  for (let index = 0; index < binary.length; index++) {
    bytes[index] = binary.charCodeAt(index)
  }
  return new Blob([bytes])
}

function randomBlobKeySuffix(length = 8): string {
  const alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'
  let suffix = ''
  for (let index = 0; index < length; index++) {
    suffix += alphabet[Math.floor(Math.random() * alphabet.length)]
  }
  return suffix
}

export async function mockUploadBlob(prefix: string, bytes: Blob | ArrayBuffer): Promise<BlobUploadResponse> {
  const blob = bytes instanceof Blob ? bytes : new Blob([bytes])
  const base64 = arrayBufferToBase64(await blob.arrayBuffer())
  const key = `${prefix}/${randomBlobKeySuffix()}`
  mutateRegistry(db => {
    db.blobs[key] = base64
    return null
  })
  return { success: true, key }
}

export function mockFetchBlob(key: string): Promise<Blob> {
  const base64 = loadMockDatabase().blobs[key]
  if (base64 === undefined) {
    throw new ApiClientError('blob not found', 'NOT_FOUND', 404, null)
  }
  return Promise.resolve(base64ToBlob(base64))
}

export function mockDeleteBlob(key: string): Promise<void> {
  mutateRegistry(db => {
    delete db.blobs[key]
    return null
  })
  return Promise.resolve()
}

function publishTimeStatus(db: ReturnType<typeof createSeedMockDatabase>): void {
  publishRealtimeMessage({
    topic: 'time.status',
    revision: db.registryRevision,
    payload: {
      synced: db.time.synced,
      currentEpochUtc: db.time.lastSyncEpochUtc,
      timezoneId: db.time.timezoneId,
    },
  })
}

export function mockFetchTimeStatus(): TimeStatusResponse {
  const db = loadMockDatabase()
  const nowEpochUtc = Math.floor(Date.now() / 1000)
  return ok({
    enabled: db.time.enabled,
    synced: db.time.synced,
    waitingForStation: db.time.enabled && !db.time.synced,
    ntpServer: db.time.ntpServer,
    timezoneId: db.time.timezoneId,
    syncIntervalSeconds: db.time.syncIntervalSeconds,
    source: db.time.source,
    ...(db.time.synced
      ? {
          currentEpochUtc: nowEpochUtc,
          lastSyncEpochUtc: db.time.lastSyncEpochUtc,
          localTimeIso8601: new Date(nowEpochUtc * 1000).toISOString(),
          utcOffsetMinutes: 0,
          timezoneAbbrev: 'UTC',
        }
      : {}),
  })
}

export function mockFetchTimeSettings(): TimeSettingsRecord {
  const db = loadMockDatabase()
  return ok({
    enabled: db.time.enabled,
    ntpServer: db.time.ntpServer,
    timezoneId: db.time.timezoneId,
    syncIntervalSeconds: db.time.syncIntervalSeconds,
  })
}

export function mockUpdateTimeSettings(settings: Partial<TimeSettingsRecord>): Promise<TimeSettingsRecord> {
  const response = mutateRegistry(db => {
    db.time = {
      ...db.time,
      ...(settings.enabled !== undefined ? { enabled: settings.enabled } : {}),
      ...(settings.ntpServer !== undefined ? { ntpServer: settings.ntpServer } : {}),
      ...(settings.timezoneId !== undefined ? { timezoneId: settings.timezoneId } : {}),
      ...(settings.syncIntervalSeconds !== undefined ? { syncIntervalSeconds: settings.syncIntervalSeconds } : {}),
    }
    // Every settings save (even a no-op one) forces an immediate resync in the real firmware -
    // simulate that here only when NTP is actually enabled.
    if (db.time.enabled) {
      db.time.synced = true
      db.time.source = 'ntp'
      db.time.lastSyncEpochUtc = Math.floor(Date.now() / 1000)
    }
    return mockFetchTimeSettings()
  })
  publishTimeStatus(loadMockDatabase())
  return Promise.resolve(response)
}

export function mockFetchPersistenceSettings(): PersistenceSettingsRecord {
  const db = loadMockDatabase()
  return ok({
    debounceMs: db.persistence.debounceMs,
    maxDelayMs: db.persistence.maxDelayMs,
  })
}

export function mockUpdatePersistenceSettings(settings: Partial<PersistenceSettingsRecord>): Promise<PersistenceSettingsRecord> {
  const response = mutateRegistry(db => {
    db.persistence = {
      ...db.persistence,
      ...(settings.debounceMs !== undefined ? { debounceMs: settings.debounceMs } : {}),
      ...(settings.maxDelayMs !== undefined ? { maxDelayMs: settings.maxDelayMs } : {}),
    }
    // Build the response from the just-mutated `db` directly rather than calling
    // mockFetchPersistenceSettings() (which re-reads via loadMockDatabase()): mutateRegistry only
    // writes `db` back to storage *after* this mutator returns, so a nested fresh read here would
    // echo back the pre-mutation value.
    return ok({ debounceMs: db.persistence.debounceMs, maxDelayMs: db.persistence.maxDelayMs })
  })
  return Promise.resolve(response)
}

export function mockFlushDevicePersistence(): Promise<DeviceFlushResponse> {
  const db = loadMockDatabase()
  return Promise.resolve({
    success: true,
    registryRevision: db.registryRevision,
    pendingPersistence: false,
  })
}

export function mockSetSystemTime(payload: { iso8601: string }): Promise<TimeStatusResponse> {
  const parsedEpochUtc = Math.floor(new Date(payload.iso8601).getTime() / 1000)
  mutateRegistry(db => {
    if (Number.isFinite(parsedEpochUtc)) {
      db.time.synced = true
      db.time.source = 'manual'
      db.time.lastSyncEpochUtc = parsedEpochUtc
    }
    return null
  })
  publishTimeStatus(loadMockDatabase())
  return Promise.resolve(mockFetchTimeStatus())
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

function normalizeDependencyDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isInteger(numeric) && numeric > 0 ? numeric : 0
}

function normalizeThermostatDependencyLinks(value: unknown, fallbackConfig: unknown = null): DeviceDependencyLink[] {
  if (Array.isArray(value)) {
    // Parsing untyped mock-persisted JSON (analogous to a wire boundary): role is only known to
    // be a non-empty string here, not yet narrowed to DeviceRole.
    const links = value
      .filter(isRecordPayload)
      .map(item => ({
        role: typeof item.role === 'string' ? item.role.trim() : '',
        deviceId: normalizeDependencyDeviceId(item.deviceId),
      }))
      .filter(item => item.role.length > 0 && item.deviceId > 0) as DeviceDependencyLink[]
    if (links.length > 0) {
      return links.filter(item => item.role === 'temperature_sensor' || item.role === 'switch')
    }
  }

  if (isRecordPayload(fallbackConfig)) {
    const temperatureSensorId = normalizeDependencyDeviceId(fallbackConfig.temperatureSensorDeviceId)
    const switchDeviceId = normalizeDependencyDeviceId(fallbackConfig.switchDeviceId)
    const links: DeviceDependencyLink[] = []
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
  const targetCelsius = normalizeFiniteNumber(current.targetCelsius, normalizeFiniteNumber(current.targetMilliCelsius, 25000) / 1000)
  const minSafeCelsius = normalizeFiniteNumber(current.minSafeCelsius, normalizeFiniteNumber(current.minSafeMilliCelsius, 0) / 1000)
  const maxSafeCelsius = normalizeFiniteNumber(current.maxSafeCelsius, normalizeFiniteNumber(current.maxSafeMilliCelsius, 50000) / 1000)
  const hysteresisCelsius = Math.max(0, normalizeFiniteNumber(current.hysteresisCelsius, normalizeFiniteNumber(current.hysteresisCentiCelsius, 50) / 100))
  return {
    enabled: typeof current.enabled === 'boolean' ? current.enabled : enabledFallback,
    mode: normalizeThermostatMode(current.mode),
    algorithm: normalizeThermostatAlgorithm(current.algorithm),
    targetCelsius,
    targetMilliCelsius: Math.round(targetCelsius * 1000),
    minSafeCelsius,
    minSafeMilliCelsius: Math.round(minSafeCelsius * 1000),
    maxSafeCelsius,
    maxSafeMilliCelsius: Math.round(maxSafeCelsius * 1000),
    hysteresisCelsius,
    hysteresisCentiCelsius: Math.round(hysteresisCelsius * 100),
    checkIntervalMs: Math.max(250, Math.round(normalizeFiniteNumber(current.checkIntervalMs, 1000))),
    sensorTimeoutMs: Math.max(250, Math.round(normalizeFiniteNumber(current.sensorTimeoutMs, 6000))),
    retryAfterErrorMs: Math.max(250, Math.round(normalizeFiniteNumber(current.retryAfterErrorMs, 30000))),
    minSwitchIntervalMs: Math.max(0, Math.round(normalizeFiniteNumber(current.minSwitchIntervalMs, 5000))),
  }
}

function requireThermostatDependencies(db: ReturnType<typeof createSeedMockDatabase>, deps: DeviceDependencyLink[]): void {
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
  const sensorDeviceId = dependencyDeviceIdForRole(deps as DeviceDependencyLink[], 'temperature_sensor')
  const switchDeviceId = dependencyDeviceIdForRole(deps as DeviceDependencyLink[], 'switch')
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
  let desiredSwitchState = false
  let controlStatus = 'ready'

  if (!Boolean(config.enabled)) {
    desiredSwitchState = false
    controlStatus = 'disabled'
  } else if (!sensor || sensor.record.typeName !== 'ds18b20_temperature_sensor' || !switchDevice || switchDevice.record.typeName !== 'gpio_switch') {
    desiredSwitchState = false
    controlStatus = 'dependency_blocked'
  } else if (!sensor.config.enabled || sensor.runtime.effectiveStatus !== 'ready' || !validTemperature) {
    desiredSwitchState = false
    controlStatus = 'sensor_timeout'
  } else if (mode === 'off') {
    desiredSwitchState = false
    controlStatus = 'idle'
  } else if (mode === 'heat') {
    if (currentTemperature <= target - hysteresis) {
      desiredSwitchState = true
      controlStatus = 'heating'
    } else if (currentTemperature >= target + hysteresis) {
      desiredSwitchState = false
      controlStatus = 'idle'
    } else {
      desiredSwitchState = (switchDevice.runtime.output as { state?: boolean } | undefined)?.state ?? false
      controlStatus = desiredSwitchState ? 'heating' : 'idle'
    }
  } else {
    if (currentTemperature >= target + hysteresis) {
      desiredSwitchState = true
      controlStatus = 'cooling'
    } else if (currentTemperature <= target - hysteresis) {
      desiredSwitchState = false
      controlStatus = 'idle'
    } else {
      desiredSwitchState = (switchDevice.runtime.output as { state?: boolean } | undefined)?.state ?? false
      controlStatus = desiredSwitchState ? 'cooling' : 'idle'
    }
  }

  const actualSwitchState = (switchDevice?.runtime.output as { state?: boolean } | undefined)?.state ?? false
  if (actualSwitchState !== desiredSwitchState && controlStatus !== 'disabled' && controlStatus !== 'dependency_blocked' && controlStatus !== 'sensor_timeout') {
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
    dependencyDeviceIdForRole((device.config.deps ?? []) as DeviceDependencyLink[], 'onewire_bus') === dependencyDeviceId &&
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
    dependencyDeviceIdForRole((device.config.deps ?? []) as DeviceDependencyLink[], 'i2c_bus') === dependencyDeviceId &&
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
    const sensorDeviceId = dependencyDeviceIdForRole((config.deps ?? []) as DeviceDependencyLink[], 'temperature_sensor')
    const switchDeviceId = dependencyDeviceIdForRole((config.deps ?? []) as DeviceDependencyLink[], 'switch')
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
  layout: Record<string, unknown> | null
}

function exportDeviceRecord(device: DeviceRecord): DeviceSetupExportRecord {
  const config = exportDeviceConfig(device)
  const layout = isRecordPayload(config.layout) ? cloneConfig(config.layout) : null
  delete config.layout
  return {
    record: {
      id: device.record.id,
      typeName: device.record.typeName,
      configRevision: device.record.configRevision,
    },
    config,
    layout,
  }
}

export function mockExportDeviceSetupBundle(): string {
  const db = loadMockDatabase()
  const lines: string[] = []
  lines.push(JSON.stringify({
    kind: 'transfer_envelope',
    transferSchemaVersion: 3,
    registrySchemaVersion: 1,
    registryRevision: db.registryRevision,
    deviceCount: db.devices.length,
  }))

  for (const device of db.devices) {
    const exported = exportDeviceRecord(device)
    lines.push(JSON.stringify({
      kind: 'device',
      record: {
        ...exported.record,
        configVersion:
          device.record.typeName === 'ssd1306'
            ? 5
            : device.record.typeName === 'htu21' || device.record.typeName === 'aht10' || device.record.typeName === 'dht11'
              ? 3
              : device.record.typeName === 'rtc_ds3231' ||
                  device.record.typeName === 'pcf8574_expander' ||
                  device.record.typeName === 'pcf8575_expander'
                ? 2
                : 1,
      },
      config: exported.config,
    }))
    if (exported.layout !== null) {
      const pages = Array.isArray(exported.layout.pages)
        ? exported.layout.pages.filter(isRecordPayload)
        : []
      lines.push(JSON.stringify({
        kind: 'layout_begin',
        deviceId: device.record.id,
        schemaVersion: Number(exported.layout.schemaVersion ?? 1),
        activePageId: String(exported.layout.activePageId ?? pages[0]?.id ?? 'main'),
        pageCount: pages.length,
      }))
      pages.forEach((page, pageIndex) => {
        const widgets = Array.isArray(page.widgets) ? page.widgets.filter(isRecordPayload) : []
        lines.push(JSON.stringify({
          kind: 'layout_page',
          deviceId: device.record.id,
          pageIndex,
          id: String(page.id ?? ''),
          name: String(page.name ?? ''),
          order: Number(page.order ?? pageIndex),
          widgetCount: widgets.length,
        }))
        widgets.forEach((widget, widgetIndex) => {
          lines.push(JSON.stringify({
            kind: 'layout_widget',
            deviceId: device.record.id,
            pageIndex,
            widgetIndex,
            ...widget,
          }))
        })
      })
      lines.push(JSON.stringify({ kind: 'layout_end', deviceId: device.record.id }))
    }
  }

  lines.push(JSON.stringify({
    kind: 'dashboard_layout',
    revision: db.dashboardLayoutRevision,
    layout: db.dashboardLayout,
  }))

  return `${lines.join('\n')}\n`
}

export function mockImportDeviceSetupBundle(file: File): Promise<DeviceSetupTransferResponse> {
  return file.text().then(text => {
    const lines = text.split(/\r?\n/).map(line => line.trim()).filter(Boolean)
    if (lines.length === 0) {
      throw new ApiClientError('bundle file is missing', 'BAD_JSON', 400, null)
    }

    const envelope = JSON.parse(lines[0]) as Record<string, unknown>
    const schemaVersion = Number(envelope.transferSchemaVersion ?? 0)
    if (envelope.kind !== 'transfer_envelope' || schemaVersion < 1 || schemaVersion > 3) {
      throw new ApiClientError('unsupported transfer schema version', 'INVALID_VERSION', 400, null)
    }

    const devices: DeviceRecord[] = []
    let dashboardLayout: DashboardLayoutRecord | null = null
    let currentDevice: DeviceRecord | null = null
    let pendingLayout: {
      deviceId: number
      schemaVersion: number
      activePageId: string
      pageCount: number
      pages: Array<Record<string, unknown> & { widgets: Record<string, unknown>[] }>
    } | null = null
    for (const line of lines.slice(1)) {
      const parsed = JSON.parse(line) as Record<string, unknown>
      if (parsed.kind === 'layout_begin') {
        if (schemaVersion < 3 || currentDevice === null || pendingLayout !== null || Number(parsed.deviceId) !== currentDevice.record.id) {
          throw new ApiClientError('display layout_begin record is out of order', 'BAD_ARGS', 400, null)
        }
        pendingLayout = {
          deviceId: currentDevice.record.id,
          schemaVersion: Number(parsed.schemaVersion ?? 1),
          activePageId: String(parsed.activePageId ?? ''),
          pageCount: Number(parsed.pageCount ?? 0),
          pages: [],
        }
        continue
      }
      if (parsed.kind === 'layout_page') {
        if (pendingLayout === null || Number(parsed.deviceId) !== pendingLayout.deviceId
          || Number(parsed.pageIndex) !== pendingLayout.pages.length) {
          throw new ApiClientError('display layout_page record is out of order', 'BAD_ARGS', 400, null)
        }
        pendingLayout.pages.push({
          id: String(parsed.id ?? ''),
          name: String(parsed.name ?? ''),
          order: Number(parsed.order ?? pendingLayout.pages.length),
          widgets: [],
          widgetCount: Number(parsed.widgetCount ?? 0),
        })
        continue
      }
      if (parsed.kind === 'layout_widget') {
        const pageIndex = Number(parsed.pageIndex)
        const page = pendingLayout?.pages[pageIndex]
        if (pendingLayout === null || page === undefined || Number(parsed.deviceId) !== pendingLayout.deviceId
          || Number(parsed.widgetIndex) !== page.widgets.length) {
          throw new ApiClientError('display layout_widget record is out of order', 'BAD_ARGS', 400, null)
        }
        const widget = cloneConfig(parsed)
        delete widget.kind
        delete widget.deviceId
        delete widget.pageIndex
        delete widget.widgetIndex
        page.widgets.push(widget)
        continue
      }
      if (parsed.kind === 'layout_end') {
        if (pendingLayout === null || currentDevice === null || Number(parsed.deviceId) !== pendingLayout.deviceId
          || pendingLayout.pages.length !== pendingLayout.pageCount
          || pendingLayout.pages.some(page => page.widgets.length !== Number(page.widgetCount))) {
          throw new ApiClientError('display layout is incomplete', 'BAD_ARGS', 400, null)
        }
        currentDevice.config.layout = {
          schemaVersion: pendingLayout.schemaVersion,
          activePageId: pendingLayout.activePageId,
          pages: pendingLayout.pages.map(({ widgetCount: _widgetCount, ...page }) => page),
        }
        pendingLayout = null
        continue
      }
      if (parsed.kind === 'dashboard_layout') {
        if (pendingLayout !== null) {
          throw new ApiClientError('display layout is incomplete', 'BAD_ARGS', 400, null)
        }
        dashboardLayout = (parsed.layout ?? null) as DashboardLayoutRecord | null
        continue
      }
      if (parsed.kind !== 'device') {
        throw new ApiClientError('unexpected bundle record kind', 'BAD_ARGS', 400, null)
      }
      if (pendingLayout !== null) {
        throw new ApiClientError('display layout is incomplete', 'BAD_ARGS', 400, null)
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
      currentDevice = createDeviceRecord(
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
      )
      devices.push(currentDevice)
    }
    if (pendingLayout !== null) {
      throw new ApiClientError('display layout is incomplete', 'BAD_ARGS', 400, null)
    }

    mutateRegistry(db => {
      db.devices = devices
      db.registryRevision = Number(envelope.registryRevision ?? db.registryRevision)
      if (dashboardLayout !== null) {
        const nextLayout = normalizeDashboardLayout(dashboardLayout, devices.map(device => device.record.id))
        // A no-op import (re-importing the bundle that was just exported) must not churn the
        // revision - export/import round-trips stay byte-identical.
        if (JSON.stringify(nextLayout) !== JSON.stringify(db.dashboardLayout)) {
          db.dashboardLayoutRevision += 1
        }
        db.dashboardLayout = nextLayout
      }
      refreshMockDerivedDeviceState(db)
      return null
    })

    return ok({
      registryRevision: Number(envelope.registryRevision ?? 0),
      deviceCount: devices.length,
      warnings: [],
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

// ============================================================================
// Dosing pump mock runtime
// ============================================================================

interface MockDosingRunState {
  startedAtMs: number
  endsAtMs: number
  targetMl: number
  logging: boolean
}

function roundMockMl(value: number): number {
  return Math.round(value * 100) / 100
}

function mockDosingSpeed(device: MockDeviceRecord): number {
  return Math.max(0.001, normalizeFiniteNumber((device.config as Record<string, unknown>).dosingSpeedMlPerSec, 1))
}

function mockDosingOutput(device: MockDeviceRecord): Record<string, unknown> {
  return (isRecordPayload(device.runtime.output) ? device.runtime.output : {}) as Record<string, unknown>
}

// Recomputes the derived container snapshot after a volume change - the mock's stand-in for
// DosingPumpDeviceApiAdapter::writeRuntimeJson()'s percent/empty/status derivation.
function mockDosingContainerSnapshot(device: MockDeviceRecord, currentMl: number): DosingPumpContainerSnapshot {
  const config = device.config as Record<string, unknown>
  const container = isRecordPayload(config.container) ? config.container : {}
  const capacityMl = Math.max(1, normalizeFiniteNumber(container.capacityMl, 1000))
  const thresholdPercent = Math.min(100, Math.max(0, normalizeFiniteNumber(container.thresholdPercent, 10)))
  const bounded = Math.min(capacityMl, Math.max(0, roundMockMl(currentMl)))
  const percent = Math.round((bounded / capacityMl) * 100)
  const sensorPresent = Array.isArray(config.deps)
    ? (config.deps as DeviceDependencyLink[]).some(link => link.role === 'condition')
    : false
  const empty = bounded <= 0
  return {
    capacityMl,
    currentMl: bounded,
    percent,
    empty,
    sensorPresent,
    status: empty ? 'critical' : percent <= thresholdPercent ? 'warning' : 'normal',
  }
}

export function refreshMockDosingDerivedOutput(device: MockDeviceRecord, deps: DeviceDependencyLink[]): void {
  void deps
  const output = mockDosingOutput(device)
  const config = device.config as Record<string, unknown>
  const schedule = isRecordPayload(config.schedule) ? config.schedule : {}
  const doses = Array.isArray(schedule.doses) ? schedule.doses : []
  const currentContainer = (isRecordPayload(output.container) ? output.container : {}) as DosingPumpContainerSnapshot
  const container = mockDosingContainerSnapshot(device, normalizeFiniteNumber(currentContainer.currentMl, Number.MAX_SAFE_INTEGER))
  const todayTargetMl = roundMockMl(doses.reduce((sum: number, dose) =>
    sum + (isRecordPayload(dose) ? normalizeFiniteNumber(dose.amountMl, 0) : 0), 0))
  const skipNext = Array.from({ length: doses.length }, (_, index) =>
    Array.isArray(output.skipNext) ? output.skipNext[index] === true : false)
  device.runtime.output = { ...output, container, todayTargetMl, skipNext } as DosingPumpOutputSnapshot
}

export function startMockDosingRun(device: MockDeviceRecord, amountMl: number, logging: boolean): void {
  const output = mockDosingOutput(device)
  if (output.state === 'dosing') {
    throw new ApiClientError('dosing pump is busy', 'INVALID_COMMAND', 400, null)
  }
  const speed = mockDosingSpeed(device)
  const totalSec = amountMl / speed
  const now = Date.now()
  ;(device.runtime as Record<string, unknown>).mockDosingRun = {
    startedAtMs: now,
    endsAtMs: now + totalSec * 1000,
    targetMl: amountMl,
    logging,
  } satisfies MockDosingRunState
  device.runtime.output = {
    ...output,
    state: 'dosing',
    doseType: logging ? 'manual' : 'calibration',
    dosingTargetMl: roundMockMl(amountMl),
    dosedMl: 0,
    dosingRemainingSec: Math.ceil(totalSec),
    dosingTotalSec: Math.ceil(totalSec),
  } as DosingPumpOutputSnapshot
}

export function finishMockDosingRun(db: ReturnType<typeof loadMockDatabase>, device: MockDeviceRecord, nowMs: number): void {
  const runtime = device.runtime as Record<string, unknown>
  const run = runtime.mockDosingRun as MockDosingRunState | undefined
  const output = mockDosingOutput(device)
  delete runtime.mockDosingRun
  if (!run) {
    // stopDose is idempotent - stopping an idle pump is a no-op, mirroring the firmware.
    device.runtime.output = { ...output, state: 'idle' } as DosingPumpOutputSnapshot
    return
  }
  const speed = mockDosingSpeed(device)
  const elapsedSec = Math.max(0, (Math.min(nowMs, run.endsAtMs) - run.startedAtMs) / 1000)
  const dosedMl = Math.min(run.targetMl, roundMockMl(elapsedSec * speed))
  const currentContainer = (isRecordPayload(output.container) ? output.container : {}) as DosingPumpContainerSnapshot
  const container = mockDosingContainerSnapshot(device, normalizeFiniteNumber(currentContainer.currentMl, 0) - dosedMl)

  const next: Record<string, unknown> = { ...output, state: 'idle', lastRunDosedMl: dosedMl, container }
  delete next.doseType
  delete next.dosingTargetMl
  delete next.dosedMl
  delete next.dosingRemainingSec
  delete next.dosingTotalSec
  if (run.logging && dosedMl > 0) {
    const at = nowLocalFlavoredEpochSeconds()
    next.todayDosedMl = roundMockMl(normalizeFiniteNumber(output.todayDosedMl, 0) + dosedMl)
    next.lastDose = { at, type: 'manual', amountMl: dosedMl }
    db.doseJournal.push({ deviceId: device.record.id, at, type: 'manual', amountMl: dosedMl })
    if (db.doseJournal.length > 3000) {
      db.doseJournal = db.doseJournal.slice(-2500)
    }
  }
  device.runtime.output = next as DosingPumpOutputSnapshot
}

export function setMockDosingContainerVolume(device: MockDeviceRecord, volumeMl: number): void {
  const output = mockDosingOutput(device)
  device.runtime.output = {
    ...output,
    container: mockDosingContainerSnapshot(device, volumeMl),
  } as DosingPumpOutputSnapshot
}

// Advances active mock dosing runs one simulation tick: live progress while running, completion
// (with container/journal bookkeeping) once the run's deadline passes. Returns the ids of devices
// whose runtime changed so the caller can publish device.upsert for them.
export function tickMockDosingRuns(db: ReturnType<typeof loadMockDatabase>, nowMs: number): number[] {
  const updatedIds: number[] = []
  for (const device of db.devices) {
    if (device.record.typeName !== 'dosing_pump') {
      continue
    }
    const run = (device.runtime as Record<string, unknown>).mockDosingRun as MockDosingRunState | undefined
    if (!run) {
      continue
    }
    if (nowMs >= run.endsAtMs) {
      finishMockDosingRun(db, device, nowMs)
    } else {
      const speed = mockDosingSpeed(device)
      const elapsedSec = Math.max(0, (nowMs - run.startedAtMs) / 1000)
      device.runtime.output = {
        ...mockDosingOutput(device),
        dosedMl: Math.min(run.targetMl, roundMockMl(elapsedSec * speed)),
        dosingRemainingSec: Math.max(0, Math.ceil((run.endsAtMs - nowMs) / 1000)),
      } as DosingPumpOutputSnapshot
    }
    updatedIds.push(device.record.id)
  }
  return updatedIds
}

export function mockFetchDoseJournal(deviceId: number, periodDays: number): DoseJournalResponse {
  const db = loadMockDatabase()
  const boundedPeriodDays = Math.min(365, Math.max(1, Math.round(periodDays) || 7))
  const sinceEpoch = nowLocalFlavoredEpochSeconds() - boundedPeriodDays * 86400
  const entries = db.doseJournal
    .filter((entry: MockDoseJournalEntry) => (deviceId <= 0 || entry.deviceId === deviceId) && entry.at >= sinceEpoch)
    .sort((a: MockDoseJournalEntry, b: MockDoseJournalEntry) => b.at - a.at)
    .slice(0, 1000)
    .map((entry: MockDoseJournalEntry) => ({ at: entry.at, type: entry.type, amountMl: entry.amountMl }))
  return { entries, success: true }
}

const MOCK_MAX_SCHEDULE_PRESETS = 3

export function mockFetchSchedulePresets(deviceId: number): SchedulePresetsResponse {
  const db = loadMockDatabase()
  const presets = Array.from({ length: MOCK_MAX_SCHEDULE_PRESETS }, (_unused, slot) => {
    const saved = db.schedulePresets.find((preset: MockSchedulePreset) => preset.deviceId === deviceId && preset.slot === slot)
    return saved ? { slot, filled: true, name: saved.name, points: saved.points } : { slot, filled: false }
  })
  return { deviceId, presets, success: true }
}

export function mockSaveSchedulePreset(deviceId: number, slot: number, name: string, points: SchedulePresetPoint[]): void {
  const db = loadMockDatabase()
  const record: MockSchedulePreset = { deviceId, slot, name, points: points.map(point => ({ ...point })) }
  db.schedulePresets = [
    ...db.schedulePresets.filter((preset: MockSchedulePreset) => !(preset.deviceId === deviceId && preset.slot === slot)),
    record,
  ]
  saveMockDatabase(db)
}

export function mockDeleteSchedulePreset(deviceId: number, slot: number): void {
  const db = loadMockDatabase()
  db.schedulePresets = db.schedulePresets.filter((preset: MockSchedulePreset) => !(preset.deviceId === deviceId && preset.slot === slot))
  saveMockDatabase(db)
}
