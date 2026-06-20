import type {
  DashboardLayoutRecord,
  DashboardLayoutWidgetRecord,
  DeviceRecord,
  OtaStatusResponse,
  WifiScanNetwork,
  WifiStatusResponse,
} from '@/api'
import { safeReadStorage, safeWriteStorage } from '@/utils/storage'

const storageKey = 'gekko.mockDb.v5'

export interface MockDatabase {
  registryRevision: number
  dashboardLayoutRevision: number
  dashboardLayout: DashboardLayoutRecord
  pendingPersistence: boolean
  devices: DeviceRecord[]
  wifi: {
    status: WifiStatusResponse['wifi_status']
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
    schema_version: 1,
    active_panel_id: 'main',
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
        ],
      },
    ],
  },
  pendingPersistence: false,
  devices: [
    {
      device_id: 670845748,
      type_id: 1,
      label: 'Dummy device',
      type: 'dummy',
      name: 'Aquarium Lamp',
      enabled: true,
      deps: [],
      has_deps: false,
      has_parent: false,
      parent_device_id: 0,
      config_version: 1,
      config_revision: 4,
      lifecycle_status: 'ready',
      effective_status: 'ready',
      status: 'ready',
      retained_state_supported: false,
      retained_state_in_config_payload: false,
      config: {
        enabled: true,
        name: 'Aquarium Lamp',
      },
    },
    {
      device_id: 670845749,
      type_id: 1,
      label: 'Dummy device',
      type: 'dummy',
      name: 'Temperature Sensor',
      enabled: true,
      deps: [],
      has_deps: false,
      has_parent: false,
      parent_device_id: 0,
      config_version: 1,
      config_revision: 2,
      lifecycle_status: 'disabled',
      effective_status: 'disabled',
      status: 'disabled',
      retained_state_supported: false,
      retained_state_in_config_payload: false,
      config: {
        enabled: true,
        name: 'Temperature Sensor',
      },
    },
    {
      device_id: 670845750,
      type_id: 2,
      label: 'GPIO switch',
      type: 'gpio_switch',
      name: 'GPIO Relay',
      enabled: true,
      deps: [],
      has_deps: false,
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
        startup_state: 'off',
        safe_state: 'disabled',
        inverted: false,
        gpio_pin: 4,
      },
      output: {
        state: 'off',
      },
    },
    {
      device_id: 670845751,
      type_id: 3,
      label: 'OneWire bus',
      type: 'onewire_bus',
      name: 'Sensor Bus',
      enabled: true,
      deps: [],
      has_deps: false,
      has_parent: false,
      parent_device_id: 0,
      config_version: 1,
      config_revision: 1,
      lifecycle_status: 'ready',
      effective_status: 'ready',
      status: 'ready',
      retained_state_supported: false,
      config: {
        enabled: true,
        gpio_pin: 18,
        internal_pullup: false,
      },
      scan: {
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
      },
    },
    {
      device_id: 670845752,
      type_id: 4,
      label: 'DS18B20 temperature sensor',
      type: 'ds18b20_temperature_sensor',
      name: 'Water Temperature',
      enabled: true,
      deps: [
        {
          role: 'onewire_bus',
          device_id: 670845751,
        },
      ],
      has_deps: true,
      has_parent: true,
      parent_device_id: 670845751,
      config_version: 1,
      config_revision: 1,
      lifecycle_status: 'ready',
      effective_status: 'ready',
      status: 'ready',
      retained_state_supported: false,
      config: {
        enabled: true,
        address: '28FF641D621603AD',
        resolution: 12,
        unit: 'celsius',
        poll_ms: 5000,
        report_delta_celsius: 0.25,
        report_always: false,
      },
      output: {
        temperature: {
          value: 24.625,
          unit: 'celsius',
          unit_symbol: 'C',
          measured_at_ms: 18500,
          valid: true,
          status: 'ok',
        },
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
    free_sketch_space: 1900544,
    has_error: false,
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
      Number(value.device_id ?? 0),
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
  return Array.isArray(value.panels) && typeof value.active_panel_id === 'string' && typeof value.schema_version === 'number'
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
    pendingPersistence: typeof stored.pendingPersistence === 'boolean' ? stored.pendingPersistence : seed.pendingPersistence,
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
