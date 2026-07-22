import type {
  BaseDeviceConfig,
  BaseDeviceRuntime,
  DashboardLayoutRecord,
  DashboardLayoutWidgetRecord,
  DeviceHaSettings,
  DeviceRecord,
  DeviceOutputSnapshot,
  DoseJournalEntry,
  I2cBusScanSnapshot,
  OneWireScanSnapshot,
  OtaStatusResponse,
  SystemStatusResponse,
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

// Mirrors the firmware's HaEntityAdapterRegistry::withDefaults() (src/integrations/mqtt/HaEntityAdapter.cpp)
// - keep in sync whenever a new IHaEntityAdapter is added there.
const HA_SUPPORTED_TYPE_NAMES = new Set([
  'gpio_switch',
  'ds18b20_temperature_sensor',
  'ntc_thermistor_temperature_sensor',
  'htu21',
  'thermostat',
  'port_expander_switch',
  'analog_output',
  'fade_analog_output',
  'scheduled_analog_output',
])

export function isHaSupportedTypeName(typeName: string): boolean {
  return HA_SUPPORTED_TYPE_NAMES.has(typeName)
}

export const storageKey = 'gekko.mockDb.v11'

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

// Per-pump dose history entries, mirroring the firmware's LittleFS dose journal shape plus the
// deviceId the REST endpoint filters by.
export interface MockDoseJournalEntry extends DoseJournalEntry {
  deviceId: number
}

export interface MockDatabase {
  registryRevision: number
  dashboardLayoutRevision: number
  dashboardLayout: DashboardLayoutRecord
  devices: MockDeviceRecord[]
  doseJournal: MockDoseJournalEntry[]
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
  systemStatus: SystemStatusResponse
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
  time: {
    enabled: boolean
    synced: boolean
    source: 'ntp' | 'manual'
    ntpServer: string
    timezoneId: string
    syncIntervalSeconds: number
    lastSyncEpochUtc: number
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

// Local-flavored epoch seconds (browser-local wall clock encoded as if it were UTC) - the same
// flavor the firmware journal and dosing runtime use, so mock timestamps render identically.
export function nowLocalFlavoredEpochSeconds(now: Date = new Date()): number {
  return Math.floor((now.getTime() - now.getTimezoneOffset() * 60000) / 1000)
}

const kSeedDosingPumpId = 670845772
const kSeedAnalogOutputId = 670845777
const kSeedAnalogComposerId = 670845792
const seedJournalNow = nowLocalFlavoredEpochSeconds()

function seedComposableAnalogOutputDevices(): MockDeviceRecord[] {
  const physicalIds = [kSeedAnalogOutputId, 670845778, 670845779, 670845780, 670845781]
  const fadeIds = [670845782, 670845783, 670845784, 670845785, 670845786]
  const scheduledIds = [670845787, 670845788, 670845789, 670845790, 670845791]
  const channelNames = ['Royal blue', 'Blue', 'White', 'Violet', 'Moonlight']
  const peakLevels = [100, 90, 55, 70, 12]
  const devices: MockDeviceRecord[] = []

  for (let index = 1; index < physicalIds.length; index += 1) {
    devices.push(createDeviceRecord(physicalIds[index], 'analog_output', 1, {
      enabled: true,
      name: `${channelNames[index]} LEDC`,
      deps: [],
      restorePreviousState: false,
      startupState: 0,
      safeState: 0,
      pin: 12 + index,
      ledcChannel: index,
      frequencyHz: 5000,
      dutyBits: 12,
      inverted: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: { state: 0 },
    }))
  }

  fadeIds.forEach((id, index) => {
    devices.push(createDeviceRecord(id, 'fade_analog_output', 1, {
      enabled: true,
      name: `${channelNames[index]} fade`,
      deps: [{ role: 'analog_output', deviceId: physicalIds[index] }],
      maxStep: 1,
      stepIntervalMs: 200,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: { state: 0, targetState: 0, transitioning: false },
    }))
  })

  scheduledIds.forEach((id, index) => {
    devices.push(createDeviceRecord(id, 'scheduled_analog_output', 1, {
      enabled: true,
      name: channelNames[index],
      deps: [{ role: 'analog_output', deviceId: fadeIds[index] }],
      points: [
        { deleted: false, minuteOfDay: 0, state: 0 },
        { deleted: false, minuteOfDay: 480, state: 0 },
        { deleted: false, minuteOfDay: 720, state: peakLevels[index] },
        { deleted: false, minuteOfDay: 1080, state: Math.round(peakLevels[index] * 0.6) },
        { deleted: false, minuteOfDay: 1320, state: 0 },
      ],
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: { state: 0, requestedState: 0, mode: 'scheduled', timeValid: true },
    }))
  })

  devices.push(createDeviceRecord(kSeedAnalogComposerId, 'analog_output_composer', 1, {
    enabled: true,
    name: 'Aquarium light',
    deps: scheduledIds.map(deviceId => ({ role: 'analog_output', deviceId })),
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
    output: {
      mode: 'scheduled',
    },
  }))

  return devices
}

// A few days of realistic history for the seeded "Calcium" pump: the daily scheduled doses plus
// an occasional manual top-up, newest last (the handler sorts newest-first on read).
function seedDoseJournalEntries(): MockDoseJournalEntry[] {
  const entries: MockDoseJournalEntry[] = []
  const doseMinutes = [8 * 60, 12 * 60, 16 * 60, 20 * 60]
  const startOfToday = seedJournalNow - (seedJournalNow % 86400)
  for (let dayOffset = 6; dayOffset >= 0; dayOffset -= 1) {
    const dayStart = startOfToday - dayOffset * 86400
    for (const minute of doseMinutes) {
      const at = dayStart + minute * 60
      if (at > seedJournalNow) {
        continue
      }
      entries.push({ deviceId: kSeedDosingPumpId, at, type: 'schedule', amountMl: 12.5 })
    }
    if (dayOffset === 3) {
      entries.push({ deviceId: kSeedDosingPumpId, at: dayStart + 14 * 3600, type: 'manual', amountMl: 5 })
    }
  }
  return entries.sort((a, b) => a.at - b.at)
}

const seedDatabase: SeedDatabase = {
  registryRevision: 24,
  dashboardLayoutRevision: 1,
  dashboardLayout: {
    schemaVersion: 1,
    activePanelId: 'buses',
    panels: [
      {
        id: 'buses',
        name: 'Buses',
        order: 0,
        widgets: [
          [670845751, 0, 0, 1, 1],
          [670845754, 1, 0, 1, 1],
          [670845757, 2, 0, 1, 1],
        ],
      },
      {
        id: 'analog-input',
        name: 'Analog input',
        order: 1,
        widgets: [
          [670845793, 0, 0, 1, 1],
          [670845794, 1, 0, 1, 1],
          [670845795, 2, 0, 1, 1],
          [670845796, 3, 0, 1, 1],
          [670845797, 4, 0, 1, 1],
          [670845798, 5, 0, 1, 1],
        ],
      },
      {
        id: 'switches',
        name: 'Switches',
        order: 2,
        widgets: [
          [670845760, 0, 0, 1, 1],
          [670845761, 1, 0, 1, 1],
          [670845762, 2, 0, 1, 1],
          [670845799, 3, 0, 1, 1],
          [670845763, 4, 0, 1, 1],
          [670845750, 5, 0, 1, 1],
          [670845764, 6, 0, 1, 1],
          [670845765, 7, 0, 1, 1],
        ],
      },
      {
        id: 'temperature-sensors',
        name: 'Temperature sensors',
        order: 3,
        widgets: [
          [670845758, 0, 0, 1, 1],
          [670845766, 1, 0, 1, 1],
          [670845752, 2, 0, 1, 1],
        ],
      },
      {
        id: 'dosing-pump',
        name: 'Dosing pump',
        order: 4,
        widgets: [
          [kSeedDosingPumpId, 0, 0, 1, 1],
          [670845774, 1, 0, 1, 1],
          [670845776, 2, 0, 1, 1],
          [670845801, 3, 0, 1, 1],
          [670845770, 4, 0, 1, 1],
          [670845773, 5, 0, 1, 1],
          [670845775, 6, 0, 1, 1],
          [670845800, 7, 0, 1, 1],
          [670845771, 8, 0, 1, 1],
        ],
      },
      {
        id: 'analog-output',
        name: 'Analog output',
        order: 5,
        widgets: [
          [kSeedAnalogOutputId, 0, 0, 1, 1],
          [670845778, 1, 0, 1, 1],
          [670845779, 2, 0, 1, 1],
          [670845780, 3, 0, 1, 1],
          [670845781, 4, 0, 1, 1],
          [670845782, 5, 0, 1, 1],
          [670845783, 6, 0, 1, 1],
          [670845784, 7, 0, 1, 1],
          [670845785, 8, 0, 1, 1],
          [670845786, 9, 0, 1, 1],
        ],
      },
      {
        id: 'aquarium-lamp',
        name: 'Aquarium lamp',
        order: 6,
        widgets: [
          [kSeedAnalogComposerId, 0, 0, 1, 1],
          [670845787, 1, 0, 1, 1],
          [670845788, 2, 0, 1, 1],
          [670845789, 3, 0, 1, 1],
          [670845790, 4, 0, 1, 1],
          [670845791, 5, 0, 1, 1],
        ],
      },
      {
        id: 'misc',
        name: 'Misc',
        order: 7,
        widgets: [
          [670845748, 0, 0, 1, 1],
          [670845749, 1, 0, 1, 1],
          [670845753, 2, 0, 1, 1],
          [670845759, 3, 0, 1, 1],
          [670845755, 4, 0, 1, 1],
          [670845756, 5, 0, 1, 1],
        ],
      },
    ],
  },
  devices: [
    createDeviceRecord(670845748, 'dummy', 4, {
      enabled: true,
      name: 'Aquarium Lamp PWM',
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
      startupState: false,
      safeState: false,
      inverted: false,
      gpioPin: 4,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: false,
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
      smoothingWeight: 1,
      calibrationFactor: 1,
      calibrationOffset: 0,
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
    createDeviceRecord(670845793, 'analog_port_input', 1, {
      enabled: true,
      name: 'Air Temperature Analog Port',
      deps: [],
      gpioPin: 34,
      attenuation: '11db',
      adcSamples: 8,
      reportAlways: false,
      reportDeltaMilliVolts: 10,
      pollMs: 1000,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        analogInput: {
          milliVolts: 1780,
          rawCode: 2208,
          measuredAtMs: 18500,
          valid: true,
          status: 'ok',
        },
      },
    }),
    createDeviceRecord(670845758, 'ntc_thermistor_temperature_sensor', 1, {
      enabled: true,
      name: 'Air Temperature',
      deps: [
        {
          role: 'analog_input',
          deviceId: 670845793,
        },
      ],
      formulaMode: 'beta',
      seriesResistorOhms: 10000,
      supplyMilliVolts: 3300,
      nominalResistanceOhms: 10000,
      nominalTempCelsius: 25,
      betaCoefficient: 3950,
      steinhartA: 0,
      steinhartB: 0,
      steinhartC: 0,
      unit: 'celsius',
      pollMs: 5000,
      reportDeltaCelsius: 0.1,
      reportAlways: false,
      smoothingWeight: 1,
      calibrationFactor: 1,
      calibrationOffset: 0,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        temperature: {
          value: 23.4,
          unit: 'celsius',
          unitSymbol: 'C',
          measuredAtMs: 18500,
          valid: true,
          status: 'ok',
        },
      },
    }),
    createDeviceRecord(670845794, 'ads1115_hub', 1, {
      enabled: true,
      name: 'Analog Expansion ADC',
      deps: [
        {
          role: 'i2c_bus',
          deviceId: 670845754,
        },
      ],
      i2cAddress: 0x48,
      gain: 'fsr2048',
      dataRateSps: '128',
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
    }),
    createDeviceRecord(670845795, 'analog_input_channel', 1, {
      enabled: true,
      name: 'pH Probe Voltage',
      deps: [
        {
          role: 'analog_input_hub',
          deviceId: 670845794,
        },
      ],
      channel: 0,
      adcSamples: 4,
      reportAlways: false,
      reportDeltaMilliVolts: 5,
      pollMs: 1000,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        analogInput: {
          milliVolts: 2048,
          rawCode: 2540,
          measuredAtMs: 18500,
          valid: true,
          status: 'ok',
        },
      },
    }),
    createDeviceRecord(670845796, 'analog_input_channel', 1, {
      enabled: true,
      name: 'ORP Probe Voltage',
      deps: [
        {
          role: 'analog_input_hub',
          deviceId: 670845794,
        },
      ],
      channel: 1,
      adcSamples: 4,
      reportAlways: false,
      reportDeltaMilliVolts: 5,
      pollMs: 1000,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        analogInput: {
          milliVolts: 1523,
          rawCode: 1889,
          measuredAtMs: 18500,
          valid: true,
          status: 'ok',
        },
      },
    }),
    createDeviceRecord(670845797, 'cd74hc4067_hub', 1, {
      enabled: true,
      name: 'Analog Multiplexer',
      deps: [],
      selectPins: [16, 17, 18, 19],
      enablePin: 255,
      sigPin: 35,
      sigAttenuation: '11db',
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
    }),
    createDeviceRecord(670845798, 'analog_input_channel', 1, {
      enabled: true,
      name: 'Sump Level Probe',
      deps: [
        {
          role: 'analog_input_hub',
          deviceId: 670845797,
        },
      ],
      channel: 3,
      adcSamples: 8,
      reportAlways: false,
      reportDeltaMilliVolts: 10,
      pollMs: 1000,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        analogInput: {
          milliVolts: 980,
          rawCode: 1215,
          measuredAtMs: 18500,
          valid: true,
          status: 'ok',
        },
      },
    }),
    createDeviceRecord(670845766, 'htu21', 1, {
      enabled: true,
      name: 'Room Climate',
      i2cAddress: 0x40,
      deps: [
        {
          role: 'i2c_bus',
          deviceId: 670845754,
        },
      ],
      unit: 'celsius',
      pollMs: 5000,
      reportDeltaCelsius: 0.1,
      reportDeltaHumidity: 0.1,
      reportAlways: false,
      temperatureFilter: {
        smoothingWeight: 1,
        calibrationFactor: 1,
        calibrationOffset: 0,
      },
      humidityFilter: {
        smoothingWeight: 1,
        calibrationFactor: 1,
        calibrationOffset: 0,
      },
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        temperature: {
          value: 23.4,
          unit: 'celsius',
          unitSymbol: 'C',
          measuredAtMs: 18500,
          valid: true,
          status: 'ok',
        },
        humidity: {
          value: 45.3,
          unitSymbol: '%',
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
        desiredSwitchState: true,
        actualSwitchState: false,
        controlStatus: 'heating',
        lastCheckAtMs: 18500,
      },
    }),
    createDeviceRecord(670845759, 'rtc_ds3231', 1, {
      enabled: true,
      name: 'RTC Clock',
      deps: [
        {
          role: 'i2c_bus',
          deviceId: 670845754,
        },
      ],
      i2cAddress: 0x68,
      useForSystemTimeSync: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      currentEpochUtc: Math.floor(Date.now() / 1000),
      lastReadOk: true,
      oscillatorStopped: false,
    }),
    createDeviceRecord(670845760, 'pcf8574_expander', 1, {
      enabled: true,
      name: 'Relay Expander',
      deps: [
        {
          role: 'i2c_bus',
          deviceId: 670845754,
        },
      ],
      i2cAddress: 0x20,
      inverted: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      channelCount: 8,
      channelStates: 0b101,
      diagnostics: {
        status: 'ok',
        consecutiveErrors: 0,
        lastErrorCode: 0,
        lastErrorAtMs: 0,
        errorOps: 0,
      },
    }),
    createDeviceRecord(670845761, 'pcf8575_expander', 1, {
      enabled: true,
      name: 'Grow Light Expander',
      deps: [
        {
          role: 'i2c_bus',
          deviceId: 670845754,
        },
      ],
      i2cAddress: 0x21,
      inverted: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      channelCount: 16,
      channelStates: 0b1,
      diagnostics: {
        status: 'ok',
        consecutiveErrors: 0,
        lastErrorCode: 0,
        lastErrorAtMs: 0,
        errorOps: 0,
      },
    }),
    createDeviceRecord(670845762, 'port_expander_switch', 1, {
      enabled: true,
      name: 'Expander Channel 0',
      deps: [
        {
          role: 'port_expander',
          deviceId: 670845760,
        },
      ],
      restorePreviousState: false,
      startupState: false,
      safeState: false,
      inverted: false,
      channel: 0,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: true,
      },
    }),
    createDeviceRecord(670845799, 'port_expander_switch', 1, {
      enabled: true,
      name: 'Grow Light Expander Channel 0',
      deps: [
        {
          role: 'port_expander',
          deviceId: 670845761,
        },
      ],
      restorePreviousState: false,
      startupState: false,
      safeState: false,
      inverted: false,
      channel: 0,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: false,
      },
    }),
    createDeviceRecord(670845763, 'gpio_switch', 1, {
      enabled: true,
      name: 'Grow Light',
      deps: [],
      restorePreviousState: false,
      startupState: false,
      safeState: false,
      inverted: false,
      gpioPin: 16,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: true,
      },
    }),
    createDeviceRecord(670845764, 'schedule', 1, {
      enabled: true,
      name: 'Grow Light Schedule',
      deps: [],
      rules: [
        {
          enabled: true,
          weekDays: [0, 1, 2, 3, 4, 5, 6],
          startMinuteOfDay: 8 * 60,
          endMinuteOfDay: 20 * 60,
          mode: 'alwaysOn',
          intervalsPerWindow: 1,
          durationMinutes: 1,
        },
      ],
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
    }),
    createDeviceRecord(670845765, 'auto_switch', 1, {
      enabled: true,
      name: 'Grow Light Auto Switch',
      // Demonstrates the AND-condition list: the schedule (not inverted) plus an unrelated relay
      // used as a manual "hold off" override (inverted - satisfied only while that relay is off).
      deps: [
        {
          role: 'switch',
          deviceId: 670845763,
        },
        {
          role: 'condition',
          deviceId: 670845764,
        },
        {
          role: 'condition',
          deviceId: 670845750,
          invert: true,
        },
      ],
      pauseDurationSeconds: 1800,
      targetSwitchDeviceId: 670845763,
      conditions: [
        { deviceId: 670845764, invert: false },
        { deviceId: 670845750, invert: true },
      ],
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        mode: 'auto',
        paused: false,
        pausedUntilMs: 0,
        conditionsSatisfied: true,
        state: true,
      },
    }),
    createDeviceRecord(670845770, 'gpio_switch', 1, {
      enabled: true,
      name: 'Calcium Pump Relay',
      deps: [],
      gpioPin: 25,
      inverted: false,
      restorePreviousState: false,
      startupState: false,
      safeState: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: false,
      },
    }),
    createDeviceRecord(670845771, 'binary_sensor', 1, {
      enabled: true,
      name: 'Calcium Low Level',
      deps: [],
      gpioPin: 34,
      pullMode: 'none',
      inverted: false,
      debounceMs: 100,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        active: false,
        rawLevel: false,
        hasReading: true,
      },
    }),
    createDeviceRecord(kSeedDosingPumpId, 'dosing_pump', 1, {
      enabled: true,
      name: 'Calcium',
      deps: [
        {
          role: 'switch',
          deviceId: 670845770,
        },
        {
          role: 'condition',
          deviceId: 670845771,
        },
      ],
      dosingSpeedMlPerSec: 1.25,
      container: {
        capacityMl: 500,
        thresholdPercent: 15,
        blockAutoWhenEmpty: true,
      },
      schedule: {
        mode: 'daily',
        everyDays: 1,
        daysOfWeek: [0, 1, 2, 3, 4, 5, 6],
        anchorDay: 0,
        doses: [
          { time: '08:00', amountMl: 12.5 },
          { time: '12:00', amountMl: 12.5 },
          { time: '16:00', amountMl: 12.5 },
          { time: '20:00', amountMl: 12.5 },
        ],
      },
      pumpSwitchDeviceId: 670845770,
      levelSensorDeviceId: 670845771,
      levelSensorInvert: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: 'idle',
        autoMode: true,
        timeValid: true,
        lastRunDosedMl: 12.5,
        todayDosedMl: 25,
        todayTargetMl: 50,
        nextDoseAt: seedJournalNow - (seedJournalNow % 3600) + 3600,
        nextDoseAmountMl: 12.5,
        lastDose: {
          at: seedJournalNow - 4 * 3600,
          type: 'schedule',
          amountMl: 12.5,
        },
        daysLeft: 4,
        container: {
          capacityMl: 500,
          currentMl: 230,
          percent: 46,
          empty: false,
          sensorPresent: true,
          status: 'normal',
        },
        skipNext: [false, false, false, false],
      },
    }),
    createDeviceRecord(670845773, 'gpio_switch', 1, {
      enabled: true,
      name: 'Nopox Pump Relay',
      deps: [],
      gpioPin: 26,
      inverted: false,
      restorePreviousState: false,
      startupState: false,
      safeState: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: false,
      },
    }),
    // Seeded with an empty container so the alerts bell / critical toast path is
    // exercised straight from ?mockMode=1&mockReset=1 without manual setup.
    createDeviceRecord(670845774, 'dosing_pump', 1, {
      enabled: true,
      name: 'Nopox',
      deps: [
        {
          role: 'switch',
          deviceId: 670845773,
        },
      ],
      dosingSpeedMlPerSec: 0.9,
      container: {
        capacityMl: 250,
        thresholdPercent: 10,
        blockAutoWhenEmpty: true,
      },
      schedule: {
        mode: 'daily',
        everyDays: 1,
        daysOfWeek: [0, 1, 2, 3, 4, 5, 6],
        anchorDay: 0,
        doses: [
          { time: '09:00', amountMl: 2 },
          { time: '21:00', amountMl: 2 },
        ],
      },
      pumpSwitchDeviceId: 670845773,
      levelSensorDeviceId: 0,
      levelSensorInvert: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: 'idle',
        autoMode: true,
        timeValid: true,
        lastRunDosedMl: 2,
        todayDosedMl: 0,
        todayTargetMl: 4,
        nextDoseAt: seedJournalNow - (seedJournalNow % 3600) + 7200,
        nextDoseAmountMl: 2,
        lastDose: {
          at: seedJournalNow - 26 * 3600,
          type: 'schedule',
          amountMl: 2,
        },
        daysLeft: 0,
        container: {
          capacityMl: 250,
          currentMl: 0,
          percent: 0,
          empty: true,
          sensorPresent: false,
          status: 'critical',
        },
        skipNext: [false, false],
      },
    }),
    createDeviceRecord(670845775, 'gpio_switch', 1, {
      enabled: true,
      name: 'Magnesium Pump Relay',
      deps: [],
      gpioPin: 27,
      inverted: false,
      restorePreviousState: false,
      startupState: false,
      safeState: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: false,
      },
    }),
    // Seeded just below the low-level threshold so the warning alert shows out of the box.
    createDeviceRecord(670845776, 'dosing_pump', 1, {
      enabled: true,
      name: 'Magnesium',
      deps: [
        {
          role: 'switch',
          deviceId: 670845775,
        },
      ],
      dosingSpeedMlPerSec: 1.1,
      container: {
        capacityMl: 1000,
        thresholdPercent: 10,
        blockAutoWhenEmpty: true,
      },
      schedule: {
        mode: 'weekly',
        everyDays: 1,
        daysOfWeek: [1, 3, 5],
        anchorDay: 0,
        doses: [
          { time: '10:00', amountMl: 15 },
          { time: '18:00', amountMl: 15 },
        ],
      },
      pumpSwitchDeviceId: 670845775,
      levelSensorDeviceId: 0,
      levelSensorInvert: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: 'idle',
        autoMode: true,
        timeValid: true,
        lastRunDosedMl: 15,
        todayDosedMl: 15,
        todayTargetMl: 30,
        nextDoseAt: seedJournalNow - (seedJournalNow % 3600) + 3600,
        nextDoseAmountMl: 15,
        lastDose: {
          at: seedJournalNow - 8 * 3600,
          type: 'schedule',
          amountMl: 15,
        },
        daysLeft: 2,
        container: {
          capacityMl: 1000,
          currentMl: 80,
          percent: 8,
          empty: false,
          sensorPresent: false,
          status: 'warning',
        },
        skipNext: [false, false],
      },
    }),
    createDeviceRecord(670845800, 'gpio_switch', 1, {
      enabled: true,
      name: 'Phosphate Pump Relay',
      deps: [],
      gpioPin: 33,
      inverted: false,
      restorePreviousState: false,
      startupState: false,
      safeState: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: false,
      },
    }),
    createDeviceRecord(670845801, 'dosing_pump', 1, {
      enabled: true,
      name: 'Phosphate',
      deps: [
        {
          role: 'switch',
          deviceId: 670845800,
        },
      ],
      dosingSpeedMlPerSec: 1,
      container: {
        capacityMl: 500,
        thresholdPercent: 10,
        blockAutoWhenEmpty: true,
      },
      schedule: {
        mode: 'weekly',
        everyDays: 1,
        daysOfWeek: [2, 5],
        anchorDay: 0,
        doses: [
          { time: '11:00', amountMl: 3 },
        ],
      },
      pumpSwitchDeviceId: 670845800,
      levelSensorDeviceId: 0,
      levelSensorInvert: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: 'idle',
        autoMode: true,
        timeValid: true,
        lastRunDosedMl: 3,
        todayDosedMl: 0,
        todayTargetMl: 0,
        nextDoseAt: seedJournalNow - (seedJournalNow % 3600) + 5400,
        nextDoseAmountMl: 3,
        lastDose: {
          at: seedJournalNow - 72 * 3600,
          type: 'schedule',
          amountMl: 3,
        },
        daysLeft: 12,
        container: {
          capacityMl: 500,
          currentMl: 410,
          percent: 82,
          empty: false,
          sensorPresent: false,
          status: 'normal',
        },
        skipNext: [false],
      },
    }),
    createDeviceRecord(kSeedAnalogOutputId, 'analog_output', 2, {
      enabled: true,
      name: 'Royal blue LEDC',
      deps: [],
      restorePreviousState: true,
      startupState: 35,
      safeState: 0,
      pin: 12,
      ledcChannel: 0,
      frequencyHz: 5000,
      dutyBits: 12,
      inverted: false,
    }, {
      status: 'ready',
      lifecycleStatus: 'ready',
      effectiveStatus: 'ready',
      output: {
        state: 35,
      },
    }),
    ...seedComposableAnalogOutputDevices(),
  ],
  doseJournal: seedDoseJournalEntries(),
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
  systemStatus: {
    chip: { model: 'ESP32-D0WDQ6', revision: 3, cores: 2, cpuFreqMhz: 240, flashSizeBytes: 4194304 },
    uptimeSeconds: 259200,
    resetReason: 'poweron',
    heap: { totalBytes: 327680, freeBytes: 148000, minFreeBytes: 92000, maxAllocBytes: 110000 },
    sketch: { usedBytes: 1520000, partitionBytes: 3276800 },
    partitions: [
      { label: 'app0', type: 'app', subtype: 'factory', offset: 65536, sizeBytes: 3276800 },
      { label: 'devdata', type: 'data', subtype: 'spiffs', offset: 3342336, sizeBytes: 262144 },
      { label: 'littlefs', type: 'data', subtype: 'spiffs', offset: 3604480, sizeBytes: 524288 },
      { label: 'nvs', type: 'data', subtype: 'nvs', offset: 4128768, sizeBytes: 65536 },
    ],
    filesystems: [
      { label: 'littlefs', mounted: true, totalBytes: 524288, usedBytes: 425984 },
      { label: 'devdata', mounted: true, totalBytes: 262144, usedBytes: 24576 },
    ],
    nvs: { usedEntries: 118, freeEntries: 902, totalEntries: 1020, namespaceCount: 6 },
    success: true,
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
  time: {
    enabled: true,
    synced: true,
    source: 'ntp',
    ntpServer: 'pool.ntp.org',
    timezoneId: 'Etc/GMT',
    syncIntervalSeconds: 3600,
    lastSyncEpochUtc: Math.floor(Date.now() / 1000),
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
    supported: isHaSupportedTypeName(typeName),
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
  const systemStatus = isRecord(stored.systemStatus) ? stored.systemStatus : {}
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
    doseJournal: Array.isArray(stored.doseJournal) ? (stored.doseJournal as MockDoseJournalEntry[]) : seed.doseJournal,
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
    systemStatus: {
      ...seed.systemStatus,
      ...systemStatus,
    },
    mqtt: {
      ...seed.mqtt,
      ...(isRecord(stored.mqtt) ? stored.mqtt : {}),
    },
    time: {
      ...seed.time,
      ...(isRecord(stored.time) ? stored.time : {}),
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
