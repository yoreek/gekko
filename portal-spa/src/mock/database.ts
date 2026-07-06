import type {
  BaseDeviceConfig,
  BaseDeviceRuntime,
  DashboardLayoutRecord,
  DashboardLayoutWidgetRecord,
  DeviceHaSettings,
  DeviceRecord,
  DeviceOutputSnapshot,
  I2cBusScanSnapshot,
  OneWireScanSnapshot,
  OtaStatusResponse,
  WifiScanNetwork,
  WifiStatusResponse,
} from '../api/contracts.ts'
import {
  defaultSsd1306Layout,
  defaultSsd1306Widget,
  normalizeSsd1306Layout,
} from '../models/devices/ssd1306/layout.ts'
import {
  defaultDisplayWidget,
} from '../models/devices/display/layout-normalizer.ts'
import { ST7735_DISPLAY_LAYOUT_PROFILE } from '../models/devices/display/profile.ts'
import { Rgb565RasterImageCodec } from '../raster/rgb565/Rgb565RasterImageCodec.ts'
import {
  defaultSt7735Layout,
  normalizeSt7735Layout,
} from '../models/devices/st7735/layout.ts'
import { safeReadStorage, safeWriteStorage } from '../utils/storage.ts'

const storageKey = 'gekko.mockDb.v7'

type MockDeviceConfig = BaseDeviceConfig & Record<string, unknown>
type MockDeviceRuntime = BaseDeviceRuntime & {
  dependencyStatus?: string
  output?: DeviceOutputSnapshot
  scan?: OneWireScanSnapshot | I2cBusScanSnapshot
  [key: string]: unknown
}

export type MockDeviceRecord = DeviceRecord<MockDeviceConfig, MockDeviceRuntime>

type SeedDatabase = Omit<MockDatabase, 'devices'> & {
  devices: MockDeviceRecord[]
}

const seededRgb565BitmapData = new Rgb565RasterImageCodec().placeholder(16, 16).toBase64()

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
  mqtt: {
    compiledIn: boolean
    enabled: boolean
    connected: boolean
    waitingForStation: boolean
    host: string
    port: number
    useTls: boolean
    clientId: string
    username: string
    password: string
    haDiscoveryPrefix: string
    haNodeId: string
    haNodeName: string
    hasCaCert: boolean
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
          [670845756, 8, 0, 1, 1],
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
      diagnostics: {
        status: 'ok',
        consecutiveErrors: 0,
        lastErrorCode: 0,
        lastErrorAtMs: 0,
        errorOps: 0,
      },
      scan: {
        inProgress: false,
        ready: false,
        deviceCount: 0,
        truncated: false,
        nextAddress: 0x08,
        devices: [],
      },
    }),
    createDeviceRecord(670845757, 'spi_bus', 1, {
      enabled: true,
      name: 'SPI Bus',
      deps: [],
      host: 2,
      sckPin: 18,
      mosiPin: 23,
      misoPin: -1,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      generation: 1,
      transactionActive: false,
      diagnostics: {
        status: 'ok',
        consecutiveErrors: 0,
        lastErrorCode: 0,
        lastErrorAtMs: 0,
        errorOps: 0,
      },
    }),
    createDeviceRecord(670845755, 'ssd1306', 1, {
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
      width: 128,
      height: 64,
      layout: {
        ...defaultSsd1306Layout(),
        pages: [
          {
            id: 'main',
            name: 'Main',
            order: 0,
            widgets: [
              {
                ...defaultSsd1306Widget('text', 0),
                id: 'text-0',
                x: 0,
                y: 0,
                width: 42,
                height: 12,
                text: 'ABC',
              },
              {
                ...defaultSsd1306Widget('bitmap', 1),
                id: 'bitmap-1',
                x: 46,
                y: 0,
                width: 16,
                height: 16,
                bitmapData: 'AAAAAAAAB+AIEBQoEAgQCBAIEAgX6A/wB+AAAAAAAAA=',
                keepAspectRatio: true,
              },
              {
                ...defaultSsd1306Widget('circle', 2),
                id: 'circle-2',
                x: 0,
                y: 18,
                width: 18,
                height: 18,
              },
              {
                ...defaultSsd1306Widget('line', 3),
                id: 'line-3',
                x: 28,
                y: 24,
                width: 36,
                height: 1,
              },
              {
                ...defaultSsd1306Widget('ellipse', 4),
                id: 'ellipse-4',
                x: 72,
                y: 16,
                width: 32,
                height: 18,
              },
            ],
          },
        ],
      },
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
    }),
    createDeviceRecord(670845756, 'st7735', 1, {
      enabled: true,
      name: 'TFT Display',
      deps: [
        {
          role: 'spi_bus',
          deviceId: 670845757,
        },
      ],
      spiBusDeviceId: 670845757,
      chipSelectPin: 5,
      dcPin: 2,
      resetPin: -1,
      width: 128,
      height: 160,
      layout: {
        ...defaultSt7735Layout(),
        pages: [
          {
            id: 'main',
            name: 'Main',
            order: 0,
            widgets: [
              {
                ...defaultDisplayWidget(ST7735_DISPLAY_LAYOUT_PROFILE, 'text', 0),
                id: 'title-0',
                x: 4,
                y: 4,
                width: 80,
                height: 18,
                text: 'TFT Demo',
                fontSize: 2,
              },
              {
                ...defaultDisplayWidget(ST7735_DISPLAY_LAYOUT_PROFILE, 'bitmap', 1),
                id: 'bitmap-2',
                x: 32,
                y: 24,
                width: 16,
                height: 16,
                bitmapData: seededRgb565BitmapData,
                keepAspectRatio: true,
              },
              {
                ...defaultDisplayWidget(ST7735_DISPLAY_LAYOUT_PROFILE, 'rect', 2),
                id: 'rect-3',
                x: 62,
                y: 26,
                width: 46,
                height: 24,
                styleFlags: {
                  filled: false,
                  inverted: false,
                  wrap: false,
                },
              },
              {
                ...defaultDisplayWidget(ST7735_DISPLAY_LAYOUT_PROFILE, 'line', 3),
                id: 'line-4',
                x: 8,
                y: 78,
                width: 96,
                height: 1,
              },
              {
                ...defaultDisplayWidget(ST7735_DISPLAY_LAYOUT_PROFILE, 'circle', 4),
                id: 'circle-5',
                x: 12,
                y: 92,
                width: 24,
                height: 24,
                styleFlags: {
                  filled: false,
                  inverted: false,
                  wrap: false,
                },
              },
              {
                ...defaultDisplayWidget(ST7735_DISPLAY_LAYOUT_PROFILE, 'ellipse', 5),
                id: 'ellipse-6',
                x: 48,
                y: 88,
                width: 52,
                height: 28,
                styleFlags: {
                  filled: false,
                  inverted: false,
                  wrap: false,
                },
              },
            ],
          },
        ],
        colorMode: 'rgb565',
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
      targetCelsius: 25,
      targetMilliCelsius: 25000,
      minSafeCelsius: 0,
      minSafeMilliCelsius: 0,
      maxSafeCelsius: 50,
      maxSafeMilliCelsius: 50000,
      hysteresisCelsius: 0.5,
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
  mqtt: {
    compiledIn: true,
    enabled: false,
    connected: false,
    waitingForStation: false,
    host: '',
    port: 1883,
    useTls: false,
    clientId: '',
    username: '',
    password: '',
    haDiscoveryPrefix: 'homeassistant',
    haNodeId: 'gekko-mock',
    haNodeName: 'Gekko (mock)',
    hasCaCert: false,
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
  if (typeName === 'ssd1306') {
    config.layout = normalizeSsd1306Layout(configSource.layout ?? defaultSsd1306Layout())
  } else if (typeName === 'st7735') {
    config.layout = normalizeSt7735Layout(configSource.layout ?? defaultSt7735Layout())
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

  const haSource = isRecord(source.ha) ? source.ha : {}
  const haEnabled = typeof haSource.enabled === 'boolean' ? haSource.enabled : false
  const haName = typeof haSource.name === 'string' ? haSource.name : ''
  const ha: DeviceHaSettings = {
    enabled: haEnabled,
    name: haName,
    effectiveName: haName.length > 0 ? haName : config.name,
  }

  return {
    record: {
      id,
      typeName,
      configRevision,
    },
    config,
    runtime,
    ha,
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
    mqtt: {
      ...seed.mqtt,
      ...(isRecord(stored.mqtt) ? stored.mqtt : {}),
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
