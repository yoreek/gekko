import type { DeviceRecord, OtaStatusResponse, WifiScanNetwork, WifiStatusResponse } from '@/api'
import { safeReadStorage, safeWriteStorage } from '@/utils/storage'

const storageKey = 'gekko.mockDb'

export interface MockDatabase {
  registryRevision: number
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
  pendingPersistence: false,
  devices: [
    {
      device_id: 670845748,
      type_id: 1,
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
      persistence_policy: 'delayed',
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
      type_id: 2,
      type: 'generic',
      name: 'Temperature Sensor',
      enabled: true,
      has_parent: false,
      parent_device_id: 0,
      config_version: 1,
      config_revision: 2,
      lifecycle_status: 'ready',
      effective_status: 'ready',
      status: 'ready',
      persistence_policy: 'delayed',
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
    return stored
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
