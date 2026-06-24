import type {
  DashboardLayoutRecord,
  DashboardLayoutWidgetRecord,
  OtaStatusResponse,
  WifiScanNetwork,
  WifiStatusResponse,
} from '@/api'
import { safeReadStorage, safeWriteStorage } from '@/utils/storage'

const storageKey = 'gekko.mockDb.v6'

type DeviceRecord = Record<string, any>

export interface MockDatabase {
  registryRevision: number
  dashboardLayoutRevision: number
  dashboardLayout: DashboardLayoutRecord
  devices: DeviceRecord[]
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

const seedDatabase: MockDatabase = {
  registryRevision: 19,
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
        ],
      },
    ],
  },
  devices: [
    {
      deviceId: 670845748,
      typeId: 1,
      label: 'Dummy device',
      typeName: 'dummy',
      name: 'Aquarium Lamp',
      enabled: true,
      deps: [],
      hasDeps: false,
      configRevision: 4,
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      status: 'ready',
      retainedStateSupported: false,
      retainedStateInConfigPayload: false,
      config: {
        enabled: true,
        name: 'Aquarium Lamp',
      },
    },
    {
      deviceId: 670845749,
      typeId: 1,
      label: 'Dummy device',
      typeName: 'dummy',
      name: 'Temperature Sensor',
      enabled: true,
      deps: [],
      hasDeps: false,
      configRevision: 2,
      lifecycleStatus: 'disabled',
      effectiveStatus: 'disabled',
      status: 'disabled',
      retainedStateSupported: false,
      retainedStateInConfigPayload: false,
      config: {
        enabled: true,
        name: 'Temperature Sensor',
      },
    },
    {
      deviceId: 670845750,
      typeId: 2,
      label: 'GPIO switch',
      typeName: 'gpio_switch',
      name: 'GPIO Relay',
      enabled: true,
      deps: [],
      hasDeps: false,
      configRevision: 1,
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      status: 'ready',
      retainedStateSupported: true,
      retainedStartupEnabled: false,
      retainedStartupFallbackOutput: false,
      retainedStateInConfigPayload: false,
      config: {
        enabled: true,
        restorePreviousState: false,
        startupState: 'off',
        safeState: 'disabled',
        inverted: false,
        gpioPin: 4,
      },
      output: {
        state: 'off',
      },
    },
    {
      deviceId: 670845751,
      typeId: 3,
      label: 'OneWire bus',
      typeName: 'onewire_bus',
      name: 'Sensor Bus',
      enabled: true,
      deps: [],
      hasDeps: false,
      configRevision: 1,
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      status: 'ready',
      retainedStateSupported: false,
      config: {
        enabled: true,
        gpioPin: 18,
        internalPullup: false,
      },
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
    },
    {
      deviceId: 670845752,
      typeId: 4,
      label: 'DS18B20 temperature sensor',
      typeName: 'ds18b20_temperature_sensor',
      name: 'Water Temperature',
      enabled: true,
      deps: [
        {
          role: 'onewire_bus',
          deviceId: 670845751,
        },
      ],
      hasDeps: true,
      configRevision: 1,
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      status: 'ready',
      retainedStateSupported: false,
      config: {
        enabled: true,
        address: '28FF641D621603AD',
        resolution: 12,
        unit: 'celsius',
        pollMs: 5000,
        reportDeltaCelsius: 0.25,
        reportAlways: false,
      },
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
    },
    {
      deviceId: 670845753,
      typeId: 5,
      label: 'Thermostat',
      typeName: 'thermostat',
      name: 'Grow Room Thermostat',
      enabled: true,
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
      hasDeps: true,
      configRevision: 1,
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      status: 'ready',
      retainedStateSupported: false,
      retainedStateInConfigPayload: false,
      config: {
        enabled: true,
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
      },
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
    },
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
  return Array.isArray(value.panels) && typeof value.activePanelId === 'string' && typeof value.schemaVersion === 'number'
}

function normalizeStoredDatabase(stored: unknown): MockDatabase {
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
    devices: Array.isArray(stored.devices) ? (stored.devices as DeviceRecord[]) : seed.devices,
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
