import type { DashboardLayoutRecord, DeviceRecord, OtaStatusResponse, WifiScanNetwork, WifiStatusResponse } from '@/api'
import { safeReadStorage, safeWriteStorage } from '@/utils/storage'

const storageKey = 'gekko.mockDb.v3'

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
  registryRevision: 18,
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
          { device_id: 670845748, x: 0, y: 0, w: 1, h: 1 },
          { device_id: 670845749, x: 1, y: 0, w: 1, h: 1 },
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
      has_parent: false,
      parent_device_id: 0,
      config_version: 1,
      config_revision: 4,
      lifecycle_status: 'ready',
      effective_status: 'ready',
      status: 'ready',
      retained_state_supported: true,
      retained_startup_enabled: true,
      retained_startup_fallback_output: false,
      retained_state_in_config_payload: false,
      config: {
        enabled: true,
        restore_previous_state: true,
        default_output: false,
        current_output: true,
        inverted: false,
      },
    },
    {
      device_id: 670845749,
      type_id: 1,
      label: 'Dummy device',
      type: 'dummy',
      name: 'Temperature Sensor',
      enabled: true,
      has_parent: false,
      parent_device_id: 0,
      config_version: 1,
      config_revision: 2,
      lifecycle_status: 'disabled',
      effective_status: 'disabled',
      status: 'disabled',
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

  return {
    ...seed,
    ...stored,
    registryRevision: typeof stored.registryRevision === 'number' ? stored.registryRevision : seed.registryRevision,
    dashboardLayoutRevision: typeof stored.dashboardLayoutRevision === 'number' ? stored.dashboardLayoutRevision : seed.dashboardLayoutRevision,
    dashboardLayout: isDashboardLayoutRecord(stored.dashboardLayout) ? stored.dashboardLayout : seed.dashboardLayout,
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
