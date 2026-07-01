import type { TemperatureOutputSnapshot } from '@/api/contracts'
import {
  defaultSsd1306Layout,
  normalizeSsd1306Layout,
} from '@/models/devices/ssd1306/layout'
import { ApiClientError } from '@/api/http'
import {
  createSeedMockDatabase,
  createDeviceRecord,
  type MockDeviceRecord,
} from './database'

type DeviceRecord = MockDeviceRecord
type Database = ReturnType<typeof createSeedMockDatabase>

// ============================================================================
// Helpers
// ============================================================================

function isRecordPayload(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

export function normalizeFiniteNumber(value: unknown, fallback: number): number {
  const numeric = Number(value)
  return Number.isFinite(numeric) ? numeric : fallback
}

function normalizeDependencyDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isInteger(numeric) && numeric > 0 ? numeric : 0
}

export function normalizeDependencyLinks(value: unknown): Array<{ role: string; deviceId: number }> {
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

export function dependencyDeviceIdForRole(deps: Array<{ role: string; deviceId: number }>, role: string): number {
  const dependency = deps.find(link => link.role === role)
  return dependency?.deviceId ?? 0
}

// ============================================================================
// Thermostat
// ============================================================================

function normalizeThermostatMode(value: unknown): 'off' | 'heat' | 'cool' {
  return value === 'heat' || value === 'cool' ? value : 'off'
}

function normalizeThermostatConfigPayload(
  value: unknown,
  enabledFallback: boolean,
): Record<string, unknown> & { enabled: boolean } {
  const current = isRecordPayload(value) ? value : {}
  return {
    enabled: typeof current.enabled === 'boolean' ? current.enabled : enabledFallback,
    mode: normalizeThermostatMode(current.mode),
    algorithm: 'hysteresis',
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

function buildThermostatOutput(
  db: Database,
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

function requireThermostatDependencies(db: Database, deps: Array<{ role: string; deviceId: number }>): void {
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

// ============================================================================
// DS18B20
// ============================================================================

function normalizeDs18b20Unit(value: unknown): 'celsius' | 'fahrenheit' {
  return value === 'fahrenheit' ? 'fahrenheit' : 'celsius'
}

function normalizeDs18b20Resolution(value: unknown): number {
  return value === 9 || value === 10 || value === 11 || value === 12 ? value : 12
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

function ds18b20AddressShapeValid(address: string): boolean {
  return /^[0-9A-Fa-f]{16}$/.test(address.trim())
}

function requireOneWireDependency(db: Database, dependencyDeviceId: number): DeviceRecord {
  const dependency = db.devices.find(entry => entry.record.id === dependencyDeviceId)
  if (!dependency || dependency.record.typeName !== 'onewire_bus') {
    throw new ApiClientError('valid onewire dependency is required', 'BAD_ARGS', 400, null)
  }
  return dependency
}

function ensureUniqueDs18b20Address(
  db: Database,
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

// ============================================================================
// I2C
// ============================================================================

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

function requireI2cDependency(db: Database, dependencyDeviceId: number): DeviceRecord {
  const dependency = db.devices.find(device => device.record.id === dependencyDeviceId)
  if (!dependency || dependency.record.typeName !== 'i2c_bus') {
    throw new ApiClientError('ssd1306 display i2c dependency is required', 'BAD_ARGS', 400, null)
  }
  return dependency
}

function ensureUniqueI2cAddress(
  db: Database,
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

// ============================================================================
// SPI
// ============================================================================

function requireSpiDependency(db: Database, dependencyDeviceId: number): DeviceRecord {
  const dependency = db.devices.find(device => device.record.id === dependencyDeviceId)
  if (!dependency || dependency.record.typeName !== 'spi_bus') {
    throw new ApiClientError('st7735 display spi dependency is required', 'BAD_ARGS', 400, null)
  }
  return dependency
}

// ============================================================================
// Factory functions
// ============================================================================

export function createGpioSwitchDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: Array<{ role: string; deviceId: number }>,
  enabled: boolean,
  name: string,
): DeviceRecord {
  return createDeviceRecord(nextId, 'gpio_switch', 1, {
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
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
    output: {
      state: 'off',
    },
  })
}

export function createOneWireBusDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: Array<{ role: string; deviceId: number }>,
  enabled: boolean,
  name: string,
): DeviceRecord {
  return createDeviceRecord(nextId, 'onewire_bus', 1, {
    enabled,
    name,
    deps: baseDeps,
    gpioPin: normalizeFiniteNumber(configSource.gpioPin, 4),
    internalPullup: typeof configSource.internalPullup === 'boolean' ? configSource.internalPullup : false,
  }, {
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
}

export function createI2cBusDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: Array<{ role: string; deviceId: number }>,
  enabled: boolean,
  name: string,
): DeviceRecord {
  const config = normalizeI2cBusConfigPayload(configSource, enabled)
  return createDeviceRecord(nextId, 'i2c_bus', 1, {
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
}

export function createSsd1306Device(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: Array<{ role: string; deviceId: number }>,
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
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
  return createDeviceRecord(nextId, 'ssd1306', 1, config, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
  })
}

export function createSt7735Device(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: Array<{ role: string; deviceId: number }>,
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
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
  return createDeviceRecord(nextId, 'st7735', 1, config, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
  })
}

export function createDs18b20Device(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: Array<{ role: string; deviceId: number }>,
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  const deps = normalizeDependencyLinks(configSource.deps)
  const dependencyDeviceId = dependencyDeviceIdForRole(deps, 'onewire_bus')
  if (dependencyDeviceId <= 0) {
    throw new ApiClientError('ds18b20 dependency is required', 'BAD_ARGS', 400, null)
  }
  requireOneWireDependency(db, dependencyDeviceId)
  const config = normalizeDs18b20ConfigPayload(configSource, enabled)
  ensureUniqueDs18b20Address(db, dependencyDeviceId, String(config.address), nextId)
  return createDeviceRecord(nextId, 'ds18b20_temperature_sensor', 1, {
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
}

export function createThermostatDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: Array<{ role: string; deviceId: number }>,
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  const deps = normalizeThermostatDependencyLinks(configSource.deps, configSource)
  const config = normalizeThermostatConfigPayload(configSource, enabled)
  requireThermostatDependencies(db, deps)
  return createDeviceRecord(nextId, 'thermostat', 1, {
    ...config,
    deps,
    name,
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
    output: buildThermostatOutput(db, config, nextId),
  })
}

export function createDummyDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: Array<{ role: string; deviceId: number }>,
  enabled: boolean,
  name: string,
): DeviceRecord {
  return createDeviceRecord(nextId, 'dummy', 1, {
    enabled,
    name,
    deps: baseDeps,
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
  })
}
