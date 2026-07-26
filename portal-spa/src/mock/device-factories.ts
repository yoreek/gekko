import type { AnalogOutputOutputSnapshot, DeviceDependencyLink, ScheduleRuleConfig, TemperatureOutputSnapshot } from '@/api/contracts'
import { isScheduleActiveAt } from '@/models/devices/schedule-preview'
import { analogInputHubChannelCount } from '@/models/devices/analog-input-channel'
import {
  defaultSsd1306Layout,
  normalizeSsd1306Layout,
} from '@/models/devices/ssd1306/layout'
import { normalizeDisplayRotation } from '@/models/devices/display/orientation'
import { isKnownPanel, resolvePanelGeometry, ST7735_DEFAULT_PANEL, SSD1306_DEFAULT_PANEL } from '@/models/devices/display/panels'
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

export function normalizeI2cAddress(value: unknown, fallback: number): number {
  const address = normalizeFiniteNumber(value, fallback)
  if (!Number.isInteger(address) || address < 0 || address > 0x7f) {
    throw new ApiClientError('i2c address is out of bounds', 'BAD_ARGS', 400, null)
  }
  return address
}

// width/height are always derived from panel (never independently settable), matching the
// firmware's parseJson for both display types.
export function normalizeDisplayPanel(typeName: string, value: unknown, fallback: string): string {
  return isKnownPanel(typeName, value) ? value : fallback
}

function normalizeDependencyDeviceId(value: unknown): number {
  const numeric = Number(value)
  return Number.isInteger(numeric) && numeric > 0 ? numeric : 0
}

export function normalizeDependencyLinks(value: unknown): DeviceDependencyLink[] {
  if (Array.isArray(value)) {
    // Parsing untyped mock-persisted JSON (analogous to a wire boundary): role is only known to
    // be a non-empty string here, not yet narrowed to DeviceRole. `invert` is only emitted when
    // true - most roles never set it, and always emitting `invert: false` would make every
    // existing link disagree byte-for-byte with the same link before it round-tripped through
    // here (see device-setup-bundle.spec.ts).
    return value
      .filter(isRecordPayload)
      .map(item => {
        const link: { role: string; deviceId: number; invert?: boolean } = {
          role: typeof item.role === 'string' ? item.role.trim() : '',
          deviceId: normalizeDependencyDeviceId(item.deviceId),
        }
        if (item.invert === true) {
          link.invert = true
        }
        return link
      })
      .filter(item => item.role.length > 0 && item.deviceId > 0) as DeviceDependencyLink[]
  }
  return []
}

export function dependencyDeviceIdForRole(deps: DeviceDependencyLink[], role: string): number {
  const dependency = deps.find(link => link.role === role)
  return dependency?.deviceId ?? 0
}

export function dependencyLinksForRole(deps: DeviceDependencyLink[], role: string): DeviceDependencyLink[] {
  return deps.filter(link => link.role === role)
}

// ============================================================================
// Analog output
// ============================================================================

export function normalizeAnalogOutputConfigPayload(
  value: unknown,
  enabledFallback: boolean,
): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid analog output config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    restorePreviousState: typeof value.restorePreviousState === 'boolean' ? value.restorePreviousState : false,
    startupState: Math.min(100, Math.max(0, Math.round(normalizeFiniteNumber(value.startupState, 0)))),
    safeState: Math.min(100, Math.max(0, Math.round(normalizeFiniteNumber(value.safeState, 0)))),
    pin: Math.round(normalizeFiniteNumber(value.pin, 4)),
    ledcChannel: Math.round(normalizeFiniteNumber(value.ledcChannel, 0)),
    frequencyHz: Math.max(1, Math.round(normalizeFiniteNumber(value.frequencyHz, 5000))),
    dutyBits: Math.max(1, Math.round(normalizeFiniteNumber(value.dutyBits, 12))),
    inverted: typeof value.inverted === 'boolean' ? value.inverted : false,
  }
}

export function createAnalogOutputDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
): DeviceRecord {
  const config = normalizeAnalogOutputConfigPayload(configSource, enabled)
  return createDeviceRecord(nextId, 'analog_output', 2, {
    ...config,
    name,
    deps: baseDeps,
  }, {
    status: config.enabled ? 'ready' : 'disabled',
    lifecycleStatus: config.enabled ? 'ready' : 'disabled',
    effectiveStatus: config.enabled ? 'ready' : 'disabled',
    output: {
      state: normalizeFiniteNumber(config.startupState, 0),
    } as AnalogOutputOutputSnapshot,
  })
}

export function normalizeFadeAnalogOutputConfigPayload(
  value: unknown,
  enabledFallback: boolean,
): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid fade analog output config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    maxStep: Math.min(100, Math.max(1, Math.round(normalizeFiniteNumber(value.maxStep, 1)))),
    stepIntervalMs: Math.min(60000, Math.max(1, Math.round(normalizeFiniteNumber(value.stepIntervalMs, 200)))),
  }
}

export function createFadeAnalogOutputDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
): DeviceRecord {
  const config = normalizeFadeAnalogOutputConfigPayload(configSource, enabled)
  const targetState = normalizeFiniteNumber(configSource.state, 0)
  return createDeviceRecord(nextId, 'fade_analog_output', 1, {
    ...config,
    name,
    deps: baseDeps,
  }, {
    status: config.enabled ? 'ready' : 'disabled',
    lifecycleStatus: config.enabled ? 'ready' : 'disabled',
    effectiveStatus: config.enabled ? 'ready' : 'disabled',
    output: { state: targetState, targetState, transitioning: false },
  })
}

function normalizeAnalogSchedulePoints(value: unknown): Array<{ deleted: boolean; minuteOfDay: number; state: number }> {
  if (!Array.isArray(value)) {
    return []
  }
  const points = value.slice(0, 10).filter(isRecordPayload).map(point => ({
    deleted: point.deleted === true,
    minuteOfDay: Math.min(1439, Math.max(0, Math.round(normalizeFiniteNumber(point.minuteOfDay, 0)))),
    state: Math.min(100, Math.max(0, Math.round(normalizeFiniteNumber(point.state, 0)))),
  }))
  const activeMinutes = points.filter(point => !point.deleted).map(point => point.minuteOfDay)
  if (new Set(activeMinutes).size !== activeMinutes.length) {
    throw new ApiClientError('analog output schedule contains duplicate times', 'INVALID_CONFIG', 400, null)
  }
  return points
}

export function normalizeScheduledAnalogOutputConfigPayload(
  value: unknown,
  enabledFallback: boolean,
): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid scheduled analog output config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    points: normalizeAnalogSchedulePoints(value.points),
  }
}

export function createScheduledAnalogOutputDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
): DeviceRecord {
  const config = normalizeScheduledAnalogOutputConfigPayload(configSource, enabled)
  return createDeviceRecord(nextId, 'scheduled_analog_output', 1, {
    ...config,
    name,
    deps: baseDeps,
  }, {
    status: config.enabled ? 'ready' : 'disabled',
    lifecycleStatus: config.enabled ? 'ready' : 'disabled',
    effectiveStatus: config.enabled ? 'ready' : 'disabled',
    output: { state: 0, requestedState: 0, mode: 'scheduled', timeValid: true },
  })
}

export function createAnalogOutputComposerDevice(
  nextId: number,
  _configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  _db: Database,
): DeviceRecord {
  return createDeviceRecord(nextId, 'analog_output_composer', 1, {
    enabled,
    name,
    deps: baseDeps,
  }, {
    status: enabled ? 'ready' : 'disabled',
    lifecycleStatus: enabled ? 'ready' : 'disabled',
    effectiveStatus: enabled ? 'ready' : 'disabled',
    output: { mode: 'scheduled' },
  })
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
  const targetCelsius = normalizeFiniteNumber(current.targetCelsius, normalizeFiniteNumber(current.targetMilliCelsius, 25000) / 1000)
  const minSafeCelsius = normalizeFiniteNumber(current.minSafeCelsius, normalizeFiniteNumber(current.minSafeMilliCelsius, 0) / 1000)
  const maxSafeCelsius = normalizeFiniteNumber(current.maxSafeCelsius, normalizeFiniteNumber(current.maxSafeMilliCelsius, 50000) / 1000)
  const hysteresisCelsius = Math.max(0, normalizeFiniteNumber(current.hysteresisCelsius, normalizeFiniteNumber(current.hysteresisCentiCelsius, 50) / 100))
  return {
    enabled: typeof current.enabled === 'boolean' ? current.enabled : enabledFallback,
    mode: normalizeThermostatMode(current.mode),
    algorithm: 'hysteresis',
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

function normalizeThermostatDependencyLinks(value: unknown, fallbackConfig: unknown = null): DeviceDependencyLink[] {
  if (Array.isArray(value)) {
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

function buildThermostatOutput(
  db: Database,
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

function requireThermostatDependencies(db: Database, deps: DeviceDependencyLink[]): void {
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
    dependencyDeviceIdForRole((device.config.deps ?? []) as DeviceDependencyLink[], 'onewire_bus') === dependencyDeviceId &&
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
    dependencyDeviceIdForRole((device.config.deps ?? []) as DeviceDependencyLink[], 'i2c_bus') === dependencyDeviceId &&
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

function normalizeSpiBusConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid spi config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    host: normalizeFiniteNumber(value.host, 2),
    sckPin: normalizeFiniteNumber(value.sckPin, 18),
    mosiPin: normalizeFiniteNumber(value.mosiPin, 23),
    misoPin: normalizeFiniteNumber(value.misoPin, -1),
  }
}

// ============================================================================
// RTC DS3231 / PCF8574 / PCF8575 port expanders / port expander switch
//
// All four share the same "device on an I2C bus, address must not collide with a sibling"
// shape, so the collision check spans every i2c-address-bearing type rather than just its own
// type (mirroring the firmware's I2cBusDevice::hasDuplicateDependentI2cAddress, which walks ALL
// dependents of the bus regardless of concrete type). This is a superset of the older
// ssd1306-only `ensureUniqueI2cAddress` above, which is left untouched for its existing callers.
// ============================================================================

const I2C_ADDRESS_BEARING_TYPE_NAMES = new Set(['ssd1306', 'rtc_ds3231', 'pcf8574_expander', 'pcf8575_expander', 'aht10', 'htu21', 'ads1115_hub'])

function deviceI2cAddress(device: DeviceRecord): number {
  return normalizeFiniteNumber((device.config as Record<string, unknown>).i2cAddress, -1)
}

export function ensureUniqueI2cAddressAcrossTypes(
  db: Database,
  dependencyDeviceId: number,
  address: number,
  currentDeviceId: number,
): void {
  const duplicate = db.devices.some(device => (
    device.record.id !== currentDeviceId &&
    I2C_ADDRESS_BEARING_TYPE_NAMES.has(device.record.typeName) &&
    dependencyDeviceIdForRole((device.config.deps ?? []) as DeviceDependencyLink[], 'i2c_bus') === dependencyDeviceId &&
    deviceI2cAddress(device) === address
  ))
  if (duplicate) {
    throw new ApiClientError('i2c address already exists on this dependency', 'DUPLICATE_ADDRESS', 400, null)
  }
}

function normalizeRtcDs3231ConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid rtc_ds3231 config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    i2cAddress: normalizeI2cAddress(value.i2cAddress, 0x68),
    useForSystemTimeSync: typeof value.useForSystemTimeSync === 'boolean' ? value.useForSystemTimeSync : false,
  }
}

function normalizePortExpanderConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid port expander config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    i2cAddress: normalizeI2cAddress(value.i2cAddress, 0x20),
    inverted: typeof value.inverted === 'boolean' ? value.inverted : false,
  }
}

export function requirePortExpanderDependency(db: Database, dependencyDeviceId: number): DeviceRecord {
  const dependency = db.devices.find(device => device.record.id === dependencyDeviceId)
  if (!dependency || (dependency.record.typeName !== 'pcf8574_expander' && dependency.record.typeName !== 'pcf8575_expander')) {
    throw new ApiClientError('port expander switch requires a port expander dependency', 'BAD_ARGS', 400, null)
  }
  return dependency
}

export function portExpanderChannelCount(dependency: DeviceRecord): number {
  return normalizeFiniteNumber((dependency.runtime.output as Record<string, unknown> | undefined)?.channelCount, 8)
}

export function ensureUniquePortExpanderChannel(
  db: Database,
  dependencyDeviceId: number,
  channel: number,
  currentDeviceId: number,
): void {
  const duplicate = db.devices.some(device => (
    device.record.id !== currentDeviceId &&
    device.record.typeName === 'port_expander_switch' &&
    dependencyDeviceIdForRole((device.config.deps ?? []) as DeviceDependencyLink[], 'port_expander') === dependencyDeviceId &&
    normalizeFiniteNumber((device.config as Record<string, unknown>).channel, -1) === channel
  ))
  if (duplicate) {
    throw new ApiClientError('port expander channel already exists on this dependency', 'DUPLICATE_CHANNEL', 400, null)
  }
}

// ============================================================================
// Factory functions
// ============================================================================

export function createGpioSwitchDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
): DeviceRecord {
  return createDeviceRecord(nextId, 'gpio_switch', 1, {
    enabled,
    name,
    deps: baseDeps,
    restorePreviousState: typeof configSource.restorePreviousState === 'boolean' ? configSource.restorePreviousState : false,
    startupState: typeof configSource.startupState === 'boolean' ? configSource.startupState : false,
    safeState: typeof configSource.safeState === 'boolean' ? configSource.safeState : false,
    inverted: typeof configSource.inverted === 'boolean' ? configSource.inverted : false,
    gpioPin: normalizeFiniteNumber(configSource.gpioPin, 2),
  }, {
    status: enabled ? 'ready' : 'disabled',
    lifecycleStatus: enabled ? 'ready' : 'disabled',
    effectiveStatus: enabled ? 'ready' : 'disabled',
    output: {
      state: false,
    },
  })
}

export function createOneWireBusDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
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
  baseDeps: DeviceDependencyLink[],
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
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  const dependencyDeviceId = dependencyDeviceIdForRole(baseDeps, 'i2c_bus')
  if (dependencyDeviceId <= 0) {
    throw new ApiClientError('ssd1306 display i2c dependency is required', 'BAD_ARGS', 400, null)
  }
  requireI2cDependency(db, dependencyDeviceId)
  const i2cAddress = normalizeI2cAddress(configSource.i2cAddress, 60)
  ensureUniqueI2cAddress(db, dependencyDeviceId, i2cAddress, nextId)
  const layout = isRecordPayload(configSource.layout)
    ? normalizeSsd1306Layout(configSource.layout)
    : defaultSsd1306Layout()
  const panel = normalizeDisplayPanel('ssd1306', configSource.panel, SSD1306_DEFAULT_PANEL)
  const geometry = resolvePanelGeometry('ssd1306', panel)
  const config = {
    enabled,
    name,
    deps: [
      {
        role: 'i2c_bus',
        deviceId: dependencyDeviceId,
      },
    ] satisfies DeviceDependencyLink[],
    i2cAddress,
    rotation: normalizeDisplayRotation(configSource.rotation, 0),
    panel,
    width: geometry?.width ?? normalizeFiniteNumber(configSource.width, 128),
    height: geometry?.height ?? normalizeFiniteNumber(configSource.height, 64),
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
  baseDeps: DeviceDependencyLink[],
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
  const panel = normalizeDisplayPanel('st7735', configSource.panel, ST7735_DEFAULT_PANEL)
  const geometry = resolvePanelGeometry('st7735', panel)
  const config = {
    enabled,
    name,
    deps: [
      {
        role: 'spi_bus',
        deviceId: dependencyDeviceId,
      },
    ] satisfies DeviceDependencyLink[],
    spiBusDeviceId: dependencyDeviceId,
    chipSelectPin: normalizeFiniteNumber(configSource.chipSelectPin, 5),
    dcPin: normalizeFiniteNumber(configSource.dcPin, 2),
    resetPin: normalizeFiniteNumber(configSource.resetPin, -1),
    rotation: normalizeDisplayRotation(configSource.rotation, 0),
    panel,
    width: geometry?.width ?? 128,
    height: geometry?.height ?? 160,
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
  baseDeps: DeviceDependencyLink[],
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
    status: enabled ? 'ready' : 'disabled',
    lifecycleStatus: enabled ? 'ready' : 'disabled',
    effectiveStatus: enabled ? 'ready' : 'disabled',
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

const kNtcFormulaModeOptions = new Set(['beta', 'steinhart_hart'])

function normalizeNtcFormulaMode(value: unknown): string {
  return typeof value === 'string' && kNtcFormulaModeOptions.has(value) ? value : 'beta'
}

// A pure resistance->temperature calculator over an AnalogInput-role dependency -- it owns no
// ADC hardware itself, only the divider geometry and the Beta/Steinhart-Hart curve (see
// "Analog input" below for the AnalogInput-role leaves it depends on).
function normalizeNtcThermistorConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid ntc thermistor config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    formulaMode: normalizeNtcFormulaMode(value.formulaMode),
    seriesResistorOhms: Math.max(1, normalizeFiniteNumber(value.seriesResistorOhms, 10000)),
    supplyMilliVolts: Math.max(1, normalizeFiniteNumber(value.supplyMilliVolts, 3300)),
    nominalResistanceOhms: Math.max(1, normalizeFiniteNumber(value.nominalResistanceOhms, 10000)),
    nominalTempCelsius: normalizeFiniteNumber(value.nominalTempCelsius, 25),
    betaCoefficient: Math.max(1, normalizeFiniteNumber(value.betaCoefficient, 3950)),
    steinhartA: normalizeFiniteNumber(value.steinhartA, 0),
    steinhartB: normalizeFiniteNumber(value.steinhartB, 0),
    steinhartC: normalizeFiniteNumber(value.steinhartC, 0),
    unit: normalizeDs18b20Unit(value.unit),
    pollMs: Math.max(1000, normalizeFiniteNumber(value.pollMs, 5000)),
    reportDeltaCelsius: Math.max(0.01, normalizeFiniteNumber(value.reportDeltaCelsius, 0.1)),
    reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : false,
    smoothingWeight: Math.min(1, Math.max(0.01, normalizeFiniteNumber(value.smoothingWeight, 1))),
    calibrationFactor: normalizeFiniteNumber(value.calibrationFactor, 1) || 1,
    calibrationOffset: normalizeFiniteNumber(value.calibrationOffset, 0),
  }
}

function requireAnalogInputDependency(db: Database, dependencyDeviceId: number): DeviceRecord {
  const dependency = db.devices.find(device => device.record.id === dependencyDeviceId)
  if (!dependency || !ANALOG_INPUT_TYPE_NAMES.has(dependency.record.typeName)) {
    throw new ApiClientError('ntc thermistor requires an analog input dependency', 'BAD_ARGS', 400, null)
  }
  return dependency
}

export function createNtcThermistorDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  const dependencyDeviceId = dependencyDeviceIdForRole(baseDeps, 'analog_input') || normalizeDependencyDeviceId(configSource.dependencyDeviceId)
  if (dependencyDeviceId <= 0) {
    throw new ApiClientError('ntc thermistor requires an analog input dependency', 'BAD_ARGS', 400, null)
  }
  requireAnalogInputDependency(db, dependencyDeviceId)
  const config = normalizeNtcThermistorConfigPayload(configSource, enabled)
  return createDeviceRecord(nextId, 'ntc_thermistor_temperature_sensor', 1, {
    ...config,
    deps: [{ role: 'analog_input', deviceId: dependencyDeviceId }],
    name,
  }, {
    status: enabled ? 'ready' : 'disabled',
    lifecycleStatus: enabled ? 'ready' : 'disabled',
    effectiveStatus: enabled ? 'ready' : 'disabled',
    output: {
      temperature: {
        value: config.nominalTempCelsius as number,
        unit: config.unit === 'fahrenheit' ? 'fahrenheit' : 'celsius',
        unitSymbol: config.unit === 'fahrenheit' ? 'F' : 'C',
        measuredAtMs: Date.now(),
        valid: true,
        status: 'ok',
      },
    },
  })
}

// ============================================================================
// Analog input
//
// analog_port_input is a standalone AnalogInput-role leaf (no dependency). ads1115_hub and
// cd74hc4067_hub are AnalogInputHub-role hubs; analog_input_channel is the single per-channel
// AnalogInput-role leaf type that depends on whichever hub is wired up -- deliberately one type,
// not one per hub chip, mirroring the firmware's role-based, hub-implementation-agnostic
// dependency (see AnalogInputHubChannelDeviceBase/AnalogInputChannelDevice). The real channel
// bound comes from whichever hub is actually selected (analogInputHubChannelCount), not a static
// per-type constant. NtcThermistorTemperatureSensorDevice above depends on any AnalogInput-role
// leaf (analog_port_input or analog_input_channel) via the plain AnalogInput role.
// ============================================================================

const ANALOG_INPUT_TYPE_NAMES = new Set(['analog_port_input', 'analog_input_channel'])
const ANALOG_INPUT_HUB_TYPE_NAMES = new Set(['ads1115_hub', 'cd74hc4067_hub'])

const kAdcAttenuationOptions = new Set(['0db', '2_5db', '6db', '11db'])

function normalizeAdcAttenuation(value: unknown): string {
  return typeof value === 'string' && kAdcAttenuationOptions.has(value) ? value : '11db'
}

function normalizeAnalogPortInputConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid analog port input config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    gpioPin: normalizeFiniteNumber(value.gpioPin, 34),
    attenuation: normalizeAdcAttenuation(value.attenuation),
    adcSamples: Math.min(64, Math.max(1, normalizeFiniteNumber(value.adcSamples, 8))),
    reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : false,
    reportDeltaMilliVolts: Math.max(1, normalizeFiniteNumber(value.reportDeltaMilliVolts, 10)),
    pollMs: Math.max(100, normalizeFiniteNumber(value.pollMs, 1000)),
  }
}

function analogInputOutput(milliVolts: number): { analogInput: Record<string, unknown> } {
  return {
    analogInput: {
      milliVolts,
      rawCode: Math.max(0, Math.min(4095, Math.round((milliVolts / 3300) * 4095))),
      measuredAtMs: Date.now(),
      valid: true,
      status: 'ok',
    },
  }
}

export function createAnalogPortInputDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
): DeviceRecord {
  const config = normalizeAnalogPortInputConfigPayload(configSource, enabled)
  return createDeviceRecord(nextId, 'analog_port_input', 1, {
    ...config,
    deps: baseDeps,
    name,
  }, {
    status: enabled ? 'ready' : 'disabled',
    lifecycleStatus: enabled ? 'ready' : 'disabled',
    effectiveStatus: enabled ? 'ready' : 'disabled',
    output: analogInputOutput(1650),
  })
}

function normalizeAds1115GainOption(value: unknown): string {
  const options = new Set(['fsr6144', 'fsr4096', 'fsr2048', 'fsr1024', 'fsr0512', 'fsr0256'])
  return typeof value === 'string' && options.has(value) ? value : 'fsr2048'
}

function normalizeAds1115DataRateOption(value: unknown): string {
  const options = new Set(['8', '16', '32', '64', '128', '250', '475', '860'])
  return typeof value === 'string' && options.has(value) ? value : '128'
}

function normalizeAds1115HubConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid ads1115 hub config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    i2cAddress: normalizeI2cAddress(value.i2cAddress, 0x48),
    gain: normalizeAds1115GainOption(value.gain),
    dataRateSps: normalizeAds1115DataRateOption(value.dataRateSps),
  }
}

export function createAds1115HubDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  const dependencyDeviceId = dependencyDeviceIdForRole(baseDeps, 'i2c_bus') || normalizeDependencyDeviceId(configSource.dependencyDeviceId)
  if (dependencyDeviceId <= 0) {
    throw new ApiClientError('ads1115 hub i2c dependency is required', 'BAD_ARGS', 400, null)
  }
  requireI2cDependency(db, dependencyDeviceId)
  const config = normalizeAds1115HubConfigPayload(configSource, enabled)
  ensureUniqueI2cAddressAcrossTypes(db, dependencyDeviceId, normalizeFiniteNumber(config.i2cAddress, 0x48), nextId)
  return createDeviceRecord(nextId, 'ads1115_hub', 1, {
    ...config,
    name,
    deps: [{ role: 'i2c_bus', deviceId: dependencyDeviceId }],
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
  })
}

function normalizeCd74hc4067HubConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid cd74hc4067 hub config', 'BAD_ARGS', 400, null)
  }
  const rawSelectPins = Array.isArray(value.selectPins) ? value.selectPins : []
  const selectPins = [0, 1, 2, 3].map(index => normalizeFiniteNumber(rawSelectPins[index], [16, 17, 18, 19][index]))
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    selectPins,
    enablePin: normalizeFiniteNumber(value.enablePin, 0xff),
    sigPin: normalizeFiniteNumber(value.sigPin, 34),
    sigAttenuation: normalizeAdcAttenuation(value.sigAttenuation),
  }
}

export function createCd74hc4067HubDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
): DeviceRecord {
  const config = normalizeCd74hc4067HubConfigPayload(configSource, enabled)
  return createDeviceRecord(nextId, 'cd74hc4067_hub', 1, {
    ...config,
    deps: baseDeps,
    name,
  }, {
    status: enabled ? 'ready' : 'disabled',
    lifecycleStatus: enabled ? 'ready' : 'disabled',
    effectiveStatus: enabled ? 'ready' : 'disabled',
  })
}

function requireAnalogInputHubDependency(db: Database, dependencyDeviceId: number): DeviceRecord {
  const dependency = db.devices.find(device => device.record.id === dependencyDeviceId)
  if (!dependency || !ANALOG_INPUT_HUB_TYPE_NAMES.has(dependency.record.typeName)) {
    throw new ApiClientError('analog input channel requires an analog input hub dependency', 'BAD_ARGS', 400, null)
  }
  return dependency
}

function ensureUniqueAnalogInputChannel(
  db: Database,
  dependencyDeviceId: number,
  channel: number,
  currentDeviceId: number,
): void {
  const duplicate = db.devices.some(device => (
    device.record.id !== currentDeviceId &&
    ANALOG_INPUT_TYPE_NAMES.has(device.record.typeName) &&
    dependencyDeviceIdForRole((device.config.deps ?? []) as DeviceDependencyLink[], 'analog_input_hub') === dependencyDeviceId &&
    normalizeFiniteNumber((device.config as Record<string, unknown>).channel, -1) === channel
  ))
  if (duplicate) {
    throw new ApiClientError('analog input channel already exists on this dependency', 'DUPLICATE_CHANNEL', 400, null)
  }
}

function normalizeAnalogInputChannelConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid analog input channel config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    channel: Math.max(0, Math.round(normalizeFiniteNumber(value.channel, 0))),
    adcSamples: Math.min(32, Math.max(1, normalizeFiniteNumber(value.adcSamples, 4))),
    reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : false,
    reportDeltaMilliVolts: Math.max(1, normalizeFiniteNumber(value.reportDeltaMilliVolts, 10)),
    pollMs: Math.max(100, normalizeFiniteNumber(value.pollMs, 1000)),
  }
}

export function createAnalogInputChannelDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
  defaultMilliVolts = 1650,
): DeviceRecord {
  const dependencyDeviceId = dependencyDeviceIdForRole(baseDeps, 'analog_input_hub') || normalizeDependencyDeviceId(configSource.dependencyDeviceId)
  if (dependencyDeviceId <= 0) {
    throw new ApiClientError('analog_input_channel requires an analog input hub dependency', 'BAD_ARGS', 400, null)
  }
  const dependency = requireAnalogInputHubDependency(db, dependencyDeviceId)
  const config = normalizeAnalogInputChannelConfigPayload(configSource, enabled)
  const channel = config.channel as number
  if (channel >= analogInputHubChannelCount(dependency.record.typeName)) {
    throw new ApiClientError('analog_input_channel channel is out of range', 'BAD_ARGS', 400, null)
  }
  ensureUniqueAnalogInputChannel(db, dependencyDeviceId, channel, nextId)
  return createDeviceRecord(nextId, 'analog_input_channel', 1, {
    ...config,
    name,
    deps: [{ role: 'analog_input_hub', deviceId: dependencyDeviceId }],
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
    output: analogInputOutput(defaultMilliVolts),
  })
}

function normalizeSensorFilterPayload(value: unknown): Record<string, number> {
  const raw = isRecordPayload(value) ? value : {}
  return {
    smoothingWeight: Math.min(1, Math.max(0.01, normalizeFiniteNumber(raw.smoothingWeight, 1))),
    calibrationFactor: normalizeFiniteNumber(raw.calibrationFactor, 1) || 1,
    calibrationOffset: normalizeFiniteNumber(raw.calibrationOffset, 0),
  }
}

export function normalizeHtu21ConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid htu21 config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    i2cAddress: normalizeI2cAddress(value.i2cAddress, 0x40),
    unit: normalizeDs18b20Unit(value.unit),
    pollMs: Math.max(1000, normalizeFiniteNumber(value.pollMs, 5000)),
    reportDeltaCelsius: Math.max(0.01, normalizeFiniteNumber(value.reportDeltaCelsius, 0.1)),
    reportDeltaHumidity: Math.max(0.01, normalizeFiniteNumber(value.reportDeltaHumidity, 0.1)),
    reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : false,
    temperatureFilter: normalizeSensorFilterPayload(value.temperatureFilter),
    humidityFilter: normalizeSensorFilterPayload(value.humidityFilter),
  }
}

export function normalizeAht10ConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid aht10 config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    i2cAddress: normalizeI2cAddress(value.i2cAddress, 0x38),
    unit: normalizeDs18b20Unit(value.unit),
    pollMs: Math.max(1000, normalizeFiniteNumber(value.pollMs, 5000)),
    reportDeltaCelsius: Math.max(0.01, normalizeFiniteNumber(value.reportDeltaCelsius, 0.1)),
    reportDeltaHumidity: Math.max(0.01, normalizeFiniteNumber(value.reportDeltaHumidity, 0.1)),
    reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : false,
    temperatureFilter: normalizeSensorFilterPayload(value.temperatureFilter),
    humidityFilter: normalizeSensorFilterPayload(value.humidityFilter),
  }
}

export function normalizeDht11ConfigPayload(value: unknown, enabledFallback: boolean): Record<string, unknown> & { enabled: boolean } {
  if (!isRecordPayload(value)) {
    throw new ApiClientError('invalid dht11 config', 'BAD_ARGS', 400, null)
  }
  return {
    enabled: typeof value.enabled === 'boolean' ? value.enabled : enabledFallback,
    gpioPin: Math.min(39, Math.max(0, Math.round(normalizeFiniteNumber(value.gpioPin, 17)))),
    unit: normalizeDs18b20Unit(value.unit),
    pollMs: Math.max(1000, normalizeFiniteNumber(value.pollMs, 5000)),
    reportDeltaCelsius: Math.max(0.01, normalizeFiniteNumber(value.reportDeltaCelsius, 0.1)),
    reportDeltaHumidity: Math.max(0.01, normalizeFiniteNumber(value.reportDeltaHumidity, 0.1)),
    reportAlways: typeof value.reportAlways === 'boolean' ? value.reportAlways : false,
    temperatureFilter: normalizeSensorFilterPayload(value.temperatureFilter),
    humidityFilter: normalizeSensorFilterPayload(value.humidityFilter),
  }
}

export function createHtu21Device(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  const dependencyDeviceId = dependencyDeviceIdForRole(baseDeps, 'i2c_bus') || normalizeDependencyDeviceId(configSource.dependencyDeviceId)
  if (dependencyDeviceId <= 0) {
    throw new ApiClientError('htu21 i2c dependency is required', 'BAD_ARGS', 400, null)
  }
  requireI2cDependency(db, dependencyDeviceId)
  const config = normalizeHtu21ConfigPayload(configSource, enabled)
  ensureUniqueI2cAddressAcrossTypes(db, dependencyDeviceId, config.i2cAddress as number, nextId)
  return createDeviceRecord(nextId, 'htu21', 1, {
    ...config,
    name,
    deps: [{ role: 'i2c_bus', deviceId: dependencyDeviceId }],
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
    output: {
      temperature: {
        value: 23.4,
        unit: config.unit === 'fahrenheit' ? 'fahrenheit' : 'celsius',
        unitSymbol: config.unit === 'fahrenheit' ? 'F' : 'C',
        measuredAtMs: Date.now(),
        valid: true,
        status: 'ok',
      },
      humidity: {
        value: 45.3,
        unitSymbol: '%',
        measuredAtMs: Date.now(),
        valid: true,
        status: 'ok',
      },
    },
  })
}

export function createAht10Device(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  const dependencyDeviceId = dependencyDeviceIdForRole(baseDeps, 'i2c_bus') || normalizeDependencyDeviceId(configSource.dependencyDeviceId)
  if (dependencyDeviceId <= 0) {
    throw new ApiClientError('aht10 i2c dependency is required', 'BAD_ARGS', 400, null)
  }
  requireI2cDependency(db, dependencyDeviceId)
  const config = normalizeAht10ConfigPayload(configSource, enabled)
  ensureUniqueI2cAddressAcrossTypes(db, dependencyDeviceId, config.i2cAddress as number, nextId)
  return createDeviceRecord(nextId, 'aht10', 1, {
    ...config,
    name,
    deps: [{ role: 'i2c_bus', deviceId: dependencyDeviceId }],
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
    output: {
      temperature: {
        value: 23.4,
        unit: config.unit === 'fahrenheit' ? 'fahrenheit' : 'celsius',
        unitSymbol: config.unit === 'fahrenheit' ? 'F' : 'C',
        measuredAtMs: Date.now(),
        valid: true,
        status: 'ok',
      },
      humidity: {
        value: 45.3,
        unitSymbol: '%',
        measuredAtMs: Date.now(),
        valid: true,
        status: 'ok',
      },
    },
  })
}

export function createDht11Device(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  void db
  if (baseDeps.length > 0 || (Array.isArray(configSource.deps) && configSource.deps.length > 0)) {
    throw new ApiClientError('dht11 does not use dependencies', 'BAD_ARGS', 400, null)
  }
  const config = normalizeDht11ConfigPayload(configSource, enabled)
  return createDeviceRecord(nextId, 'dht11', 1, {
    ...config,
    name,
    deps: [],
  }, {
    status: enabled ? 'ready' : 'disabled',
    lifecycleStatus: enabled ? 'ready' : 'disabled',
    effectiveStatus: enabled ? 'ready' : 'disabled',
    output: {
      temperature: {
        value: 23.4,
        unit: config.unit === 'fahrenheit' ? 'fahrenheit' : 'celsius',
        unitSymbol: config.unit === 'fahrenheit' ? 'F' : 'C',
        measuredAtMs: Date.now(),
        valid: true,
        status: 'ok',
      },
      humidity: {
        value: 45.3,
        unitSymbol: '%',
        measuredAtMs: Date.now(),
        valid: true,
        status: 'ok',
      },
    },
  })
}

export function createThermostatDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
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

export function createSpiBusDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
): DeviceRecord {
  const config = normalizeSpiBusConfigPayload(configSource, enabled)
  return createDeviceRecord(nextId, 'spi_bus', 1, {
    ...config,
    name,
    deps: baseDeps,
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
  })
}

export function createRtcDs3231Device(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  const dependencyDeviceId = dependencyDeviceIdForRole(baseDeps, 'i2c_bus') || normalizeDependencyDeviceId(configSource.dependencyDeviceId)
  if (dependencyDeviceId <= 0) {
    throw new ApiClientError('rtc_ds3231 i2c dependency is required', 'BAD_ARGS', 400, null)
  }
  requireI2cDependency(db, dependencyDeviceId)
  const config = normalizeRtcDs3231ConfigPayload(configSource, enabled)
  ensureUniqueI2cAddressAcrossTypes(db, dependencyDeviceId, normalizeFiniteNumber(config.i2cAddress, 0x68), nextId)
  return createDeviceRecord(nextId, 'rtc_ds3231', 1, {
    ...config,
    name,
    deps: [{ role: 'i2c_bus', deviceId: dependencyDeviceId }],
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
    currentEpochUtc: Math.floor(Date.now() / 1000),
    lastReadOk: true,
    oscillatorStopped: false,
  })
}

function createPortExpanderDevice(
  typeName: 'pcf8574_expander' | 'pcf8575_expander',
  channelCount: number,
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  const dependencyDeviceId = dependencyDeviceIdForRole(baseDeps, 'i2c_bus') || normalizeDependencyDeviceId(configSource.dependencyDeviceId)
  if (dependencyDeviceId <= 0) {
    throw new ApiClientError(`${typeName} i2c dependency is required`, 'BAD_ARGS', 400, null)
  }
  requireI2cDependency(db, dependencyDeviceId)
  const config = normalizePortExpanderConfigPayload(configSource, enabled)
  ensureUniqueI2cAddressAcrossTypes(db, dependencyDeviceId, normalizeFiniteNumber(config.i2cAddress, 0x20), nextId)
  return createDeviceRecord(nextId, typeName, 1, {
    ...config,
    name,
    deps: [{ role: 'i2c_bus', deviceId: dependencyDeviceId }],
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
    channelCount,
    channelStates: 0,
    diagnostics: {
      status: 'ok',
      consecutiveErrors: 0,
      lastErrorCode: 0,
      lastErrorAtMs: 0,
      errorOps: 0,
    },
  })
}

export function createPcf8574ExpanderDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  return createPortExpanderDevice('pcf8574_expander', 8, nextId, configSource, baseDeps, enabled, name, db)
}

export function createPcf8575ExpanderDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  return createPortExpanderDevice('pcf8575_expander', 16, nextId, configSource, baseDeps, enabled, name, db)
}

export function createPortExpanderSwitchDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  const dependencyDeviceId = dependencyDeviceIdForRole(baseDeps, 'port_expander') || normalizeDependencyDeviceId(configSource.expanderDeviceId)
  if (dependencyDeviceId <= 0) {
    throw new ApiClientError('port expander switch requires a port expander dependency', 'BAD_ARGS', 400, null)
  }
  const dependency = requirePortExpanderDependency(db, dependencyDeviceId)
  const channel = Math.max(0, Math.round(normalizeFiniteNumber(configSource.channel, 0)))
  if (channel >= portExpanderChannelCount(dependency)) {
    throw new ApiClientError('port expander switch channel is out of range', 'BAD_ARGS', 400, null)
  }
  ensureUniquePortExpanderChannel(db, dependencyDeviceId, channel, nextId)
  return createDeviceRecord(nextId, 'port_expander_switch', 1, {
    enabled,
    name,
    deps: [{ role: 'port_expander', deviceId: dependencyDeviceId }],
    restorePreviousState: typeof configSource.restorePreviousState === 'boolean' ? configSource.restorePreviousState : false,
    startupState: typeof configSource.startupState === 'boolean' ? configSource.startupState : false,
    safeState: typeof configSource.safeState === 'boolean' ? configSource.safeState : false,
    inverted: typeof configSource.inverted === 'boolean' ? configSource.inverted : false,
    channel,
  }, {
    status: enabled ? 'ready' : 'disabled',
    lifecycleStatus: enabled ? 'ready' : 'disabled',
    effectiveStatus: enabled ? 'ready' : 'disabled',
    output: {
      state: false,
    },
  })
}

export function createDummyDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
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

// ============================================================================
// Schedule
// ============================================================================

export function normalizeScheduleRule(value: unknown): Record<string, unknown> {
  const current = isRecordPayload(value) ? value : {}
  const weekDays = Array.isArray(current.weekDays)
    ? current.weekDays.map(day => normalizeFiniteNumber(day, 0)).filter(day => day >= 0 && day <= 6)
    : [0, 1, 2, 3, 4, 5, 6]
  return {
    enabled: typeof current.enabled === 'boolean' ? current.enabled : true,
    weekDays,
    startMinuteOfDay: normalizeFiniteNumber(current.startMinuteOfDay, 8 * 60),
    endMinuteOfDay: normalizeFiniteNumber(current.endMinuteOfDay, 20 * 60),
    mode: current.mode === 'interval' ? 'interval' : 'alwaysOn',
    intervalsPerWindow: Math.max(1, normalizeFiniteNumber(current.intervalsPerWindow, 1)),
    durationMinutes: Math.max(0, normalizeFiniteNumber(current.durationMinutes, 1)),
  }
}

export function createScheduleDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
): DeviceRecord {
  const rules = Array.isArray(configSource.rules) ? configSource.rules.slice(0, 4).map(normalizeScheduleRule) : []
  return createDeviceRecord(nextId, 'schedule', 1, {
    enabled,
    name,
    deps: baseDeps,
    rules,
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
  })
}

// ============================================================================
// Auto switch
// ============================================================================

// Anything that can satisfy a Condition-role AND-dependency: a schedule (evaluated live) or
// anything with a live on/off runtime state (mirrors IStatusRuntime's providers in firmware).
const kConditionCapableTypeNames = new Set(['schedule', 'gpio_switch', 'port_expander_switch', 'auto_switch', 'binary_sensor'])
// Mirrors AutoSwitchDeviceConfig.h's kMaxAutoSwitchConditions.
const kMaxAutoSwitchConditions = 6

export function requireAutoSwitchDependencies(db: Database, deps: DeviceDependencyLink[]): void {
  const switchDeviceId = dependencyDeviceIdForRole(deps, 'switch')
  const switchDevice = db.devices.find(entry => entry.record.id === switchDeviceId)
  if (!switchDevice || (switchDevice.record.typeName !== 'gpio_switch' && switchDevice.record.typeName !== 'port_expander_switch')) {
    throw new ApiClientError('auto switch requires a switch dependency', 'BAD_ARGS', 400, null)
  }
  const conditionLinks = dependencyLinksForRole(deps, 'condition')
  if (conditionLinks.length > kMaxAutoSwitchConditions) {
    throw new ApiClientError('auto switch supports at most kMaxAutoSwitchConditions condition dependencies', 'BAD_ARGS', 400, null)
  }
  // A device must not appear twice in the same auto_switch's deps, whether as two condition links
  // or as the target switch reused as its own condition (mirrors AutoSwitchDeviceApiAdapter.cpp's
  // hasDuplicateDeviceId).
  const seenDeviceIds = new Set<number>()
  for (const link of deps) {
    if (seenDeviceIds.has(link.deviceId)) {
      throw new ApiClientError('auto switch dependency device id is duplicated', 'BAD_ARGS', 400, null)
    }
    seenDeviceIds.add(link.deviceId)
  }
  for (const link of conditionLinks) {
    const conditionDevice = db.devices.find(entry => entry.record.id === link.deviceId)
    if (!conditionDevice || !kConditionCapableTypeNames.has(conditionDevice.record.typeName)) {
      throw new ApiClientError('auto switch condition dependency is invalid', 'BAD_ARGS', 400, null)
    }
  }
}

function isConditionDeviceActive(db: Database, deviceId: number): boolean {
  const device = db.devices.find(entry => entry.record.id === deviceId)
  if (!device) {
    return false
  }
  if (device.record.typeName === 'schedule') {
    const rules = Array.isArray(device.config.rules) ? (device.config.rules as ScheduleRuleConfig[]) : []
    return isScheduleActiveAt(rules, new Date())
  }
  if (device.record.typeName === 'binary_sensor') {
    const output = isRecordPayload(device.runtime.output) ? device.runtime.output : {}
    return output.active === true
  }
  // gpio_switch / port_expander_switch / auto_switch: mirrors SwitchDeviceBase::isActive() /
  // AutoSwitchDevice::isActive() - a plain on/off read of the current runtime output state.
  const output = isRecordPayload(device.runtime.output) ? device.runtime.output : {}
  return output.state === true
}

// Mirrors AutoSwitchDevice::conditionsSatisfied(): Auto mode follows the logical AND of every
// attached Condition-role dependency (each optionally inverted), evaluated live (not a
// stored/stale flag) - the mock has no server tick loop, so this just re-evaluates each condition
// against the current mock state on demand, mirroring the real device's per-tick re-evaluation.
export function autoSwitchConditionsSatisfied(db: Database, deps: DeviceDependencyLink[]): boolean {
  const conditionLinks = dependencyLinksForRole(deps, 'condition')
  if (conditionLinks.length === 0) {
    return false
  }
  return conditionLinks.every(link => isConditionDeviceActive(db, link.deviceId) !== (link.invert ?? false))
}

export function createAutoSwitchDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  requireAutoSwitchDependencies(db, baseDeps)
  return createDeviceRecord(nextId, 'auto_switch', 1, {
    enabled,
    name,
    deps: baseDeps,
    pauseDurationSeconds: Math.max(1, normalizeFiniteNumber(configSource.pauseDurationSeconds, 3600)),
  }, {
    status: enabled ? 'ready' : 'disabled',
    lifecycleStatus: enabled ? 'ready' : 'disabled',
    effectiveStatus: enabled ? 'ready' : 'disabled',
    output: {
      mode: 'auto',
      paused: false,
      pausedUntilMs: 0,
      conditionsSatisfied: false,
      state: false,
    },
  })
}

// ============================================================================
// Binary sensor
// ============================================================================

function normalizeBinarySensorPullMode(value: unknown): 'none' | 'pullup' | 'pulldown' {
  return value === 'none' || value === 'pulldown' ? value : 'pullup'
}

export function createBinarySensorDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
): DeviceRecord {
  const gpioPin = Math.min(39, Math.max(0, Math.round(normalizeFiniteNumber(configSource.gpioPin, 4))))
  // Input-only pins 34-39 have no internal pull resistors (mirrors binarySensorPinSupportsPull()).
  const pullMode = gpioPin >= 34 ? 'none' : normalizeBinarySensorPullMode(configSource.pullMode)
  return createDeviceRecord(nextId, 'binary_sensor', 1, {
    enabled,
    name,
    deps: baseDeps,
    gpioPin,
    pullMode,
    inverted: configSource.inverted === true,
    debounceMs: Math.min(60000, Math.max(0, Math.round(normalizeFiniteNumber(configSource.debounceMs, 50)))),
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
    output: {
      active: false,
      rawLevel: false,
      hasReading: true,
    },
  })
}

// ============================================================================
// Dosing pump
// ============================================================================

export const DOSING_PUMP_MAX_MOCK_DOSES = 16

function normalizeDosingDose(value: unknown): { time: string; amountMl: number } | null {
  if (!isRecordPayload(value)) {
    return null
  }
  const time = typeof value.time === 'string' && /^\d{1,2}:\d{2}$/.test(value.time) ? value.time : ''
  const amountMl = normalizeFiniteNumber(value.amountMl, 0)
  if (!time || amountMl <= 0) {
    return null
  }
  return { time, amountMl: Math.round(amountMl * 100) / 100 }
}

export function normalizeDosingPumpConfigPayload(value: unknown, current: Record<string, unknown> = {}): Record<string, unknown> {
  const source = isRecordPayload(value) ? value : {}
  const currentContainer = isRecordPayload(current.container) ? current.container : {}
  const containerSource = isRecordPayload(source.container) ? source.container : currentContainer
  const currentSchedule = isRecordPayload(current.schedule) ? current.schedule : {}
  const scheduleSource = isRecordPayload(source.schedule) ? source.schedule : currentSchedule
  const doses = Array.isArray(scheduleSource.doses)
    ? scheduleSource.doses.map(normalizeDosingDose).filter((dose): dose is { time: string; amountMl: number } => dose !== null)
      .slice(0, DOSING_PUMP_MAX_MOCK_DOSES)
    : []
  return {
    dosingSpeedMlPerSec: Math.max(0.001, normalizeFiniteNumber(source.dosingSpeedMlPerSec, normalizeFiniteNumber(current.dosingSpeedMlPerSec, 1))),
    container: {
      capacityMl: Math.max(1, Math.round(normalizeFiniteNumber(containerSource.capacityMl, 1000))),
      thresholdPercent: Math.min(100, Math.max(0, Math.round(normalizeFiniteNumber(containerSource.thresholdPercent, 10)))),
      blockAutoWhenEmpty: containerSource.blockAutoWhenEmpty !== false,
    },
    schedule: {
      mode: scheduleSource.mode === 'weekly' ? 'weekly' : 'daily',
      everyDays: Math.min(30, Math.max(1, Math.round(normalizeFiniteNumber(scheduleSource.everyDays, 1)))),
      daysOfWeek: Array.isArray(scheduleSource.daysOfWeek)
        ? scheduleSource.daysOfWeek.map(day => normalizeFiniteNumber(day, 0)).filter(day => day >= 0 && day <= 6)
        : [0, 1, 2, 3, 4, 5, 6],
      anchorDay: Math.max(0, Math.round(normalizeFiniteNumber(scheduleSource.anchorDay, 0))),
      doses,
    },
  }
}

// Anything providing DeviceRole::Switch (a live requestOutputState target for the pump motor).
const kDosingSwitchCapableTypeNames = new Set(['gpio_switch', 'port_expander_switch', 'auto_switch'])

export function requireDosingPumpDependencies(db: Database, deps: DeviceDependencyLink[]): void {
  const switchLinks = dependencyLinksForRole(deps, 'switch')
  if (switchLinks.length !== 1) {
    throw new ApiClientError('dosing pump requires a switch dependency', 'INVALID_RELATIONSHIP', 400, null)
  }
  const switchDevice = db.devices.find(entry => entry.record.id === switchLinks[0].deviceId)
  if (!switchDevice || !kDosingSwitchCapableTypeNames.has(switchDevice.record.typeName)) {
    throw new ApiClientError('switch dependency lacks output capability', 'INVALID_RELATIONSHIP', 400, null)
  }
  const conditionLinks = dependencyLinksForRole(deps, 'condition')
  if (conditionLinks.length > 1) {
    throw new ApiClientError('dosing pump supports only one level sensor dependency', 'INVALID_RELATIONSHIP', 400, null)
  }
  for (const link of conditionLinks) {
    const conditionDevice = db.devices.find(entry => entry.record.id === link.deviceId)
    if (!conditionDevice || !kConditionCapableTypeNames.has(conditionDevice.record.typeName)) {
      throw new ApiClientError('level sensor dependency lacks status capability', 'INVALID_RELATIONSHIP', 400, null)
    }
  }
}

export function createDosingPumpDevice(
  nextId: number,
  configSource: Record<string, unknown>,
  baseDeps: DeviceDependencyLink[],
  enabled: boolean,
  name: string,
  db: Database,
): DeviceRecord {
  requireDosingPumpDependencies(db, baseDeps)
  const normalized = normalizeDosingPumpConfigPayload(configSource)
  const container = normalized.container as { capacityMl: number }
  return createDeviceRecord(nextId, 'dosing_pump', 1, {
    enabled,
    name,
    deps: baseDeps,
    ...normalized,
  }, {
    status: 'ready',
    lifecycleStatus: 'ready',
    effectiveStatus: 'ready',
    output: {
      state: 'idle',
      autoMode: false,
      timeValid: true,
      lastRunDosedMl: 0,
      todayDosedMl: 0,
      todayTargetMl: 0,
      daysLeft: undefined,
      container: {
        capacityMl: container.capacityMl,
        currentMl: container.capacityMl,
        percent: 100,
        empty: false,
        sensorPresent: dependencyLinksForRole(baseDeps, 'condition').length > 0,
        status: 'normal',
      },
      skipNext: [],
    },
  })
}
