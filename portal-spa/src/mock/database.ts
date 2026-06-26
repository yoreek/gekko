import type {
  BaseDeviceConfig,
  BaseDeviceRuntime,
  DashboardLayoutRecord,
  DashboardLayoutWidgetRecord,
  DeviceRecord,
  DeviceOutputSnapshot,
  OneWireScanSnapshot,
  OtaStatusResponse,
  WifiScanNetwork,
  WifiStatusResponse,
} from '../api/contracts.ts'
import {
  defaultOledDisplayLayout,
  defaultOledDisplayWidget,
  normalizeOledDisplayLayout,
} from '../models/devices/oled-display-layout.ts'
import { safeReadStorage, safeWriteStorage } from '../utils/storage.ts'

const storageKey = 'gekko.mockDb.v6'

type MockDeviceConfig = BaseDeviceConfig & Record<string, unknown>
type MockDeviceRuntime = BaseDeviceRuntime & {
  dependencyStatus?: string
  output?: DeviceOutputSnapshot
  scan?: OneWireScanSnapshot
  [key: string]: unknown
}

export type MockDeviceRecord = DeviceRecord<MockDeviceConfig, MockDeviceRuntime>

type SeedDatabase = Omit<MockDatabase, 'devices'> & {
  devices: MockDeviceRecord[]
}

export interface MockDatabase {
  registryRevision: number
  dashboardLayoutRevision: number
  dashboardLayout: DashboardLayoutRecord
  devices: MockDeviceRecord[]
  wifi: {
    status: WifiStatusResponse['wifiStatus']
    stationIp: string
    setupApIp: string
    scan: WifiScanNetwork[]
  }
  ota: OtaStatusResponse
  system: {
    status: string
    rebooting: boolean
  }
}

export function createDeviceRecord(
  id: number,
  typeName: string,
  configRevision: number,
  config: MockDeviceConfig,
  runtime: MockDeviceRuntime,
): MockDeviceRecord {
  return {
    record: {
      id,
      typeName,
      configRevision,
    },
    config,
    runtime,
  }
}

const seedDatabase: SeedDatabase = {
  registryRevision: 20,
  dashboardLayoutRevision: 1,
  dashboardLayout: {
    schemaVersion: 1,
    activePanelId: 'main',
    panels: [
      {
        id: 'main',
        name: 'Main panel',
        order: 0,
        widgets: [
          [670845748, 0, 0, 1, 1],
          [670845749, 1, 0, 1, 1],
          [670845750, 2, 0, 1, 1],
          [670845752, 3, 0, 1, 1],
          [670845753, 4, 0, 1, 1],
          [670845751, 5, 0, 1, 1],
          [670845754, 6, 0, 1, 1],
          [670845755, 7, 0, 1, 1],
        ],
      },
    ],
  },
  devices: [
    createDeviceRecord(670845748, 'dummy', 4, {
      enabled: true,
      name: 'Aquarium Lamp',
      deps: [],
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
    }),
    createDeviceRecord(670845749, 'dummy', 2, {
      enabled: true,
      name: 'Temperature Sensor',
      deps: [],
    }, {
      status: 'disabled',
      lifecycleStatus: 'disabled',
      effectiveStatus: 'disabled',
    }),
    createDeviceRecord(670845750, 'gpio_switch', 1, {
      enabled: true,
      name: 'GPIO Relay',
      deps: [],
      restorePreviousState: false,
      startupState: 'off',
      safeState: 'disabled',
      inverted: false,
      gpioPin: 4,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: 'off',
      },
    }),
    createDeviceRecord(670845751, 'onewire_bus', 1, {
      enabled: true,
      name: 'Sensor Bus',
      deps: [],
      gpioPin: 18,
      internalPullup: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      scan: {
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
      },
    }),
    createDeviceRecord(670845754, 'i2c_bus', 1, {
      enabled: true,
      name: 'I2C Bus',
      deps: [],
      sdaPin: 21,
      sclPin: 22,
      internalPullup: true,
      frequencyHz: 100000,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
        generation: 1,
        transactionActive: false,
      }),
    createDeviceRecord(670845755, 'oled_display', 1, {
      enabled: true,
      name: 'OLED Display',
      deps: [
        {
          role: 'i2c_bus',
          deviceId: 670845754,
        },
      ],
      i2cBusDeviceId: 670845754,
      i2cAddress: 60,
      layoutWidth: 128,
      layoutHeight: 64,
      layout: {
        ...defaultOledDisplayLayout(),
        pages: [
          {
            id: 'main',
            name: 'Main',
            order: 0,
            widgets: [
              defaultOledDisplayWidget('text', 0),
            ],
          },
        ],
      },
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
    }),
    createDeviceRecord(670845752, 'ds18b20_temperature_sensor', 1, {
      enabled: true,
      name: 'Water Temperature',
      deps: [
        {
          role: 'onewire_bus',
          deviceId: 670845751,
        },
      ],
      address: '28FF641D621603AD',
      resolution: 12,
      unit: 'celsius',
      pollMs: 5000,
      reportDeltaCelsius: 0.25,
      reportAlways: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        temperature: {
          value: 24.625,
          unit: 'celsius',
          unitSymbol: 'C',
          measuredAtMs: 18500,
          valid: true,
          status: 'ok',
        },
      },
    }),
    createDeviceRecord(670845753, 'thermostat', 1, {
      enabled: true,
      name: 'Grow Room Thermostat',
      deps: [
        {
          role: 'temperature_sensor',
          deviceId: 670845752,
        },
        {
          role: 'switch',
          deviceId: 670845750,
        },
      ],
      mode: 'heat',
      algorithm: 'hysteresis',
      targetMilliCelsius: 25000,
      minSafeMilliCelsius: 0,
      maxSafeMilliCelsius: 50000,
      hysteresisCentiCelsius: 50,
      checkIntervalMs: 1000,
      sensorTimeoutMs: 6000,
      retryAfterErrorMs: 30000,
      minSwitchIntervalMs: 5000,
      temperatureSensorDeviceId: 670845752,
      switchDeviceId: 670845750,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        temperature: {
          value: 24.625,
          unit: 'celsius',
          unitSymbol: 'C',
          measuredAtMs: 18500,
          valid: true,
          status: 'ok',
        },
        desiredSwitchState: 'on',
        actualSwitchState: 'off',
        controlStatus: 'heating',
        lastCheckAtMs: 18500,
      },
    }),
  ],
  wifi: {
    status: 'connected',
    stationIp: '192.168.1.240',
    setupApIp: '192.168.4.1',
    scan: [
      { ssid: 'GekkoLab', rssi: -34, channel: 6 },
      { ssid: 'GekkoGuest', rssi: -58, channel: 11 },
      { ssid: 'OfficeMesh', rssi: -76, channel: 1 },
    ],
  },
  ota: {
    enabled: true,
    freeSketchSpace: 1900544,
    hasError: false,
    status: 'ok',
    success: true,
  },
  system: {
    status: 'idle',
    rebooting: false,
  },
}

export function canonicalizeDeviceRecord(value: unknown): MockDeviceRecord {
  const source = isRecord(value) ? value : {}
  const recordSource = isRecord(source.record) ? source.record : {}
  const configSource = isRecord(source.config) ? source.config : {}
  const runtimeSource = isRecord(source.runtime) ? source.runtime : {}
  const id = Number(recordSource.id ?? source.id ?? source.deviceId ?? 0)
  const typeName = typeof recordSource.typeName === 'string' && recordSource.typeName.trim().length > 0
    ? recordSource.typeName.trim()
    : typeof source.typeName === 'string' && source.typeName.trim().length > 0
      ? source.typeName.trim()
      : typeof source.type === 'string'
        ? source.type
        : ''
  const configRevision = Number(recordSource.configRevision ?? source.configRevision ?? 0)
  const deps = Array.isArray(configSource.deps)
    ? configSource.deps
    : Array.isArray(source.deps)
      ? source.deps
      : []
  const config: MockDeviceConfig = {
    ...configSource,
    name: typeof configSource.name === 'string' && configSource.name.length > 0
      ? configSource.name
      : typeof source.name === 'string'
        ? source.name
        : '',
    enabled: typeof configSource.enabled === 'boolean'
      ? configSource.enabled
      : typeof source.enabled === 'boolean'
        ? source.enabled
        : true,
    deps,
  }
  if (typeName === 'oled_display') {
    config.layout = normalizeOledDisplayLayout(configSource.layout ?? defaultOledDisplayLayout())
  }
  const runtimeStatus = typeof runtimeSource.status === 'string'
    ? runtimeSource.status
    : typeof source.status === 'string'
      ? source.status
      : typeof runtimeSource.effectiveStatus === 'string'
        ? runtimeSource.effectiveStatus
        : typeof runtimeSource.lifecycleStatus === 'string'
          ? runtimeSource.lifecycleStatus
          : 'unknown'
  const runtimeLifecycleStatus = typeof runtimeSource.lifecycleStatus === 'string'
    ? runtimeSource.lifecycleStatus
    : typeof runtimeSource.status === 'string'
      ? runtimeSource.status
      : typeof source.lifecycleStatus === 'string'
        ? source.lifecycleStatus
        : typeof source.status === 'string'
          ? source.status
          : runtimeStatus
  const runtimeEffectiveStatus = typeof runtimeSource.effectiveStatus === 'string'
    ? runtimeSource.effectiveStatus
    : typeof runtimeSource.status === 'string'
      ? runtimeSource.status
      : typeof source.effectiveStatus === 'string'
        ? source.effectiveStatus
        : typeof runtimeSource.lifecycleStatus === 'string'
          ? runtimeSource.lifecycleStatus
          : typeof source.lifecycleStatus === 'string'
            ? source.lifecycleStatus
            : typeof source.status === 'string'
              ? source.status
              : runtimeStatus
  const runtime: MockDeviceRuntime = {
    ...runtimeSource,
    status: runtimeStatus,
    lifecycleStatus: runtimeLifecycleStatus,
    effectiveStatus: runtimeEffectiveStatus,
  }

  if (runtimeSource.dependencyStatus === undefined && typeof source.dependencyStatus === 'string') {
    runtime.dependencyStatus = source.dependencyStatus
  }
  if (runtimeSource.output === undefined && source.output !== null && source.output !== undefined) {
    runtime.output = source.output as DeviceOutputSnapshot
  }
  if (runtimeSource.scan === undefined && source.scan !== null && source.scan !== undefined) {
    runtime.scan = source.scan as OneWireScanSnapshot
  }

  return {
    record: {
      id,
      typeName,
      configRevision,
    },
    config,
    runtime,
  }
}

function deepClone<T>(value: T): T {
  return JSON.parse(JSON.stringify(value)) as T
}

function readStoredDatabase(): MockDatabase | null {
  if (typeof window === 'undefined') {
    return null
  }

  const raw = safeReadStorage(storageKey)
  if (!raw) {
    return null
  }

  try {
    return JSON.parse(raw) as MockDatabase
  } catch {
    return null
  }
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function normalizeWidgetRecord(value: unknown): DashboardLayoutWidgetRecord | null {
  if (Array.isArray(value) && value.length >= 5) {
    return [Number(value[0]), Number(value[1]), Number(value[2]), Number(value[3]), Number(value[4])]
  }

  if (isRecord(value)) {
    return [
      Number(value.deviceId ?? 0),
      Number(value.x ?? 0),
      Number(value.y ?? 0),
      Number(value.w ?? 1),
      Number(value.h ?? 1),
    ]
  }

  return null
}

function isWidgetRecord(value: DashboardLayoutWidgetRecord | null): value is DashboardLayoutWidgetRecord {
  return value !== null
}

function isDashboardLayoutRecord(value: unknown): value is DashboardLayoutRecord {
  if (!isRecord(value)) {
    return false
  }
  return Array.isArray(value.panels) && typeof value.activePanelId === 'string' && value.schemaVersion === 1
}

export function normalizeStoredDatabase(stored: unknown): MockDatabase {
  const seed = createSeedMockDatabase()
  if (!isRecord(stored)) {
    return seed
  }

  const wifi = isRecord(stored.wifi) ? stored.wifi : {}
  const ota = isRecord(stored.ota) ? stored.ota : {}
  const system = isRecord(stored.system) ? stored.system : {}
  const dashboardLayout = isDashboardLayoutRecord(stored.dashboardLayout)
    ? {
        ...stored.dashboardLayout,
        panels: stored.dashboardLayout.panels.map(panel => ({
          ...panel,
          widgets: Array.isArray(panel.widgets)
            ? panel.widgets.map(widget => normalizeWidgetRecord(widget)).filter(isWidgetRecord)
            : [],
        })),
      }
    : seed.dashboardLayout

  return {
    ...seed,
    ...stored,
    registryRevision: typeof stored.registryRevision === 'number' ? stored.registryRevision : seed.registryRevision,
    dashboardLayoutRevision: typeof stored.dashboardLayoutRevision === 'number' ? stored.dashboardLayoutRevision : seed.dashboardLayoutRevision,
    dashboardLayout,
    devices: Array.isArray(stored.devices)
      ? stored.devices.map(device => canonicalizeDeviceRecord(device))
      : seed.devices,
    wifi: {
      ...seed.wifi,
      ...wifi,
      scan: Array.isArray(wifi.scan) ? (wifi.scan as WifiScanNetwork[]) : seed.wifi.scan,
    },
    ota: {
      ...seed.ota,
      ...ota,
    },
    system: {
      ...seed.system,
      ...system,
    },
  }
}

function writeStoredDatabase(db: MockDatabase): void {
  safeWriteStorage(storageKey, JSON.stringify(db))
}

export function createSeedMockDatabase(): MockDatabase {
  return deepClone(seedDatabase)
}

export function loadMockDatabase(reset = false): MockDatabase {
  if (reset) {
    const seed = createSeedMockDatabase()
    writeStoredDatabase(seed)
    return seed
  }

  const stored = readStoredDatabase()
  if (stored !== null) {
    return normalizeStoredDatabase(stored)
  }

  const seed = createSeedMockDatabase()
  writeStoredDatabase(seed)
  return seed
}

export function saveMockDatabase(db: MockDatabase): MockDatabase {
  const clone = deepClone(db)
  writeStoredDatabase(clone)
  return clone
}

export function resetMockDatabase(): MockDatabase {
  return loadMockDatabase(true)
}
