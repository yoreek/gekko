import type { DeviceRole } from '@/models/device-type-ids'

export interface GpioSwitchOutputSnapshot {
  state?: boolean
  physicalLevel?: boolean
}

export interface Ds18b20TemperatureSensorOutputSnapshot {
  temperature?: TemperatureOutputSnapshot
}

export interface NtcThermistorTemperatureSensorOutputSnapshot {
  temperature?: TemperatureOutputSnapshot
}

export interface Htu21SensorOutputSnapshot {
  temperature?: TemperatureOutputSnapshot
  humidity?: HumidityOutputSnapshot
}

export interface Aht10SensorOutputSnapshot {
  temperature?: TemperatureOutputSnapshot
  humidity?: HumidityOutputSnapshot
}

export interface Dht11SensorOutputSnapshot {
  temperature?: TemperatureOutputSnapshot
  humidity?: HumidityOutputSnapshot
}

export interface ThermostatOutputSnapshot {
  desiredSwitchState?: boolean
  actualSwitchState?: boolean
  controlStatus?: string
  lastCheckAtMs?: number
  temperature?: TemperatureOutputSnapshot
}

export interface RtcDs3231OutputSnapshot {
  currentEpochUtc?: number
  lastReadOk?: boolean
  oscillatorStopped?: boolean
}

export interface RtcDs1302OutputSnapshot {
  currentEpochUtc?: number
  lastReadOk?: boolean
}

export interface PortExpanderOutputSnapshot {
  channelCount?: number
  channelStates?: number
  diagnostics?: BusRuntimeDiagnosticsSnapshot
}

export type Lcd1602OutputSnapshot = Record<string, never>

export type Lcd2004OutputSnapshot = Record<string, never>

export type Lcd1602PinOutputSnapshot = Record<string, never>

export type Lcd2004PinOutputSnapshot = Record<string, never>

// Deliberately empty: the firmware never pushes a live "active"/"timeValid" value (ScheduleDevice
// never marks itself runtime-dirty, so it would just be a stale snapshot from page load) - the
// frontend instead computes an on/off preview client-side from the rule config, see
// @/models/devices/schedule-preview.ts.
export type ScheduleOutputSnapshot = Record<string, never>

export type AutoSwitchMode = 'off' | 'on' | 'auto' | 'paused'

export interface AutoSwitchOutputSnapshot {
  mode?: AutoSwitchMode
  paused?: boolean
  pausedUntilMs?: number
  conditionsSatisfied?: boolean
  state?: boolean
}

export interface BinarySensorOutputSnapshot {
  active?: boolean
  rawLevel?: boolean
  hasReading?: boolean
}

export type DosingPumpRunState = 'idle' | 'dosing'
export type DosingPumpDoseType = 'schedule' | 'manual' | 'calibration'
export type DosingPumpContainerStatus = 'normal' | 'warning' | 'critical'

export interface DosingPumpContainerSnapshot {
  capacityMl?: number
  currentMl?: number
  percent?: number
  empty?: boolean
  sensorPresent?: boolean
  status?: DosingPumpContainerStatus
}

export interface DosingPumpLastDoseSnapshot {
  at?: number
  type?: 'schedule' | 'manual'
  amountMl?: number
}

export interface DosingPumpOutputSnapshot {
  state?: DosingPumpRunState
  doseType?: DosingPumpDoseType
  dosingTargetMl?: number
  dosedMl?: number
  dosingRemainingSec?: number
  dosingTotalSec?: number
  autoMode?: boolean
  timeValid?: boolean
  lastRunDosedMl?: number
  todayDosedMl?: number
  todayTargetMl?: number
  // Local-flavored epoch seconds (device-local wall clock), matching the firmware journal.
  nextDoseAt?: number
  nextDoseAmountMl?: number
  lastDose?: DosingPumpLastDoseSnapshot
  daysLeft?: number
  container?: DosingPumpContainerSnapshot
  skipNext?: boolean[]
}

export interface AnalogOutputOutputSnapshot {
  state?: number
}

export interface FadeAnalogOutputOutputSnapshot extends AnalogOutputOutputSnapshot {
  targetState?: number
  transitioning?: boolean
}

export type AnalogOutputMode = 'off' | 'manual' | 'scheduled'

export interface ScheduledAnalogOutputOutputSnapshot extends AnalogOutputOutputSnapshot {
  requestedState?: number
  mode?: AnalogOutputMode
  timeValid?: boolean
}

export interface AnalogOutputComposerOutputSnapshot {
  mode?: AnalogOutputMode
}

export interface AnalogInputReadingSnapshot {
  milliVolts?: number
  rawCode?: number
  measuredAtMs?: number
  valid?: boolean
  status?: string
}

export interface AnalogInputOutputSnapshot {
  analogInput?: AnalogInputReadingSnapshot
}

// Hubs (ads1115_hub, cd74hc4067_hub) publish no output of their own -- they only provide channels
// -- so their runtime envelope's "output" object is always empty.
export type AnalogInputHubOutputSnapshot = Record<string, never>

// WS2812B pixel color -- RGB only, no `w`. Shared by pixel_strip's own config and every
// pixel_effect_* decorator's `color` field (see docs/pixel-strip.md).
export interface PixelColor {
  r: number
  g: number
  b: number
}

export interface PixelStripOutputSnapshot {
  pixelCount?: number
  brightness?: number
  // Explicit on/off gate -- live/retained state, independent of brightness (see
  // docs/pixel-strip.md's "on/off is an explicit gate, not derived from brightness").
  on?: boolean
}

// The live, currently-shown color -- runtime state, not config (see docs/pixel-strip.md's
// "SetOutput, not persisted config" split). config.startupColor is only what it powers up to.
export interface PixelEffectSolidOutputSnapshot {
  color?: PixelColor
  // Explicit on/off gate -- live/retained state, independent of color.
  on?: boolean
}

export interface PixelEffectAlertOutputSnapshot {
  active?: boolean
}

export interface DoseJournalEntry {
  at: number
  type: 'schedule' | 'manual'
  amountMl: number
}

export interface DoseJournalResponse {
  entries: DoseJournalEntry[]
  success?: boolean
}

// One schedule point as carried by a preset: state is a percentage (0-100), matching the
// scheduled_analog_output config JSON. `deleted` is always false for the points a preset emits.
export interface SchedulePresetPoint {
  deleted?: boolean
  minuteOfDay: number
  state: number
}

export interface SchedulePresetSlot {
  slot: number
  filled: boolean
  name?: string
  points?: SchedulePresetPoint[]
}

export interface SchedulePresetsResponse {
  deviceId: number
  presets: SchedulePresetSlot[]
  success?: boolean
}

export type DeviceOutputSnapshot =
  | GpioSwitchOutputSnapshot
  | Ds18b20TemperatureSensorOutputSnapshot
  | NtcThermistorTemperatureSensorOutputSnapshot
  | Htu21SensorOutputSnapshot
  | Aht10SensorOutputSnapshot
  | Dht11SensorOutputSnapshot
  | ThermostatOutputSnapshot
  | RtcDs3231OutputSnapshot
  | RtcDs1302OutputSnapshot
  | PortExpanderOutputSnapshot
  | ScheduleOutputSnapshot
  | AutoSwitchOutputSnapshot
  | BinarySensorOutputSnapshot
  | DosingPumpOutputSnapshot
  | AnalogOutputOutputSnapshot
  | FadeAnalogOutputOutputSnapshot
  | ScheduledAnalogOutputOutputSnapshot
  | AnalogOutputComposerOutputSnapshot
  | AnalogInputOutputSnapshot
  | AnalogInputHubOutputSnapshot
  | Lcd1602OutputSnapshot
  | Lcd2004OutputSnapshot
  | Lcd1602PinOutputSnapshot
  | Lcd2004PinOutputSnapshot
  | PixelStripOutputSnapshot
  | PixelEffectSolidOutputSnapshot
  | PixelEffectAlertOutputSnapshot

export type TemperatureUnit = 'celsius' | 'fahrenheit'

export interface TemperatureOutputSnapshot {
  value: number
  unit: TemperatureUnit
  unitSymbol: string
  measuredAtMs: number
  valid: boolean
  status?: string
}

export interface HumidityOutputSnapshot {
  value: number
  unitSymbol: string
  measuredAtMs: number
  valid: boolean
  status?: string
}

export interface OneWireScanDeviceSnapshot {
  address: string
  familyCode: string
  ownerDeviceId: number
}

export interface OneWireScanSnapshot {
  inProgress: boolean
  ready: boolean
  deviceCount: number
  truncated: boolean
  invalidCrcSeen: boolean
  devices: OneWireScanDeviceSnapshot[]
}

export interface I2cBusRuntimeSnapshot {
  generation?: number
  transactionActive?: boolean
  diagnostics?: BusRuntimeDiagnosticsSnapshot
  scan?: I2cBusScanSnapshot
}

export interface SpiBusProbeSnapshot {
  ready: boolean
  csPin: number
  outcome: 'unknown' | 'detected' | 'not_detected' | 'inconclusive'
  method: 'none' | 'miso_activity' | 'cs_pull_heuristic'
  checkedAtMs: number
}

export interface SpiBusRuntimeSnapshot {
  generation?: number
  transactionActive?: boolean
  diagnostics?: BusRuntimeDiagnosticsSnapshot
  probe?: SpiBusProbeSnapshot
}

export interface BusRuntimeDiagnosticsSnapshot {
  status?: string
  consecutiveErrors?: number
  lastErrorCode?: number
  lastErrorAtMs?: number
  errorOps?: number
}

export interface I2cBusScanDeviceSnapshot {
  address: number
  addressHex: string
  ownerDeviceId: number
}

export interface I2cBusScanSnapshot {
  inProgress: boolean
  ready: boolean
  deviceCount: number
  truncated: boolean
  nextAddress: number
  devices: I2cBusScanDeviceSnapshot[]
}

export interface DeviceDependencyLink {
  role: DeviceRole
  deviceId: number
  invert?: boolean
}

export type ScheduleRuleMode = 'alwaysOn' | 'interval'

export interface ScheduleRuleConfig {
  enabled: boolean
  weekDays: number[]
  startMinuteOfDay: number
  endMinuteOfDay: number
  mode: ScheduleRuleMode
  intervalsPerWindow: number
  durationMinutes: number
}

export interface DeviceRecordBase {
  id: number
  typeName: string
  configRevision: number
}

export interface DeviceHaSettings {
  supported: boolean
  enabled: boolean
  name: string
  effectiveName: string
}

export interface BaseDeviceConfig {
  name: string
  enabled: boolean
  deps: DeviceDependencyLink[]
}

export interface BaseDeviceRuntime {
  status: string
  lifecycleStatus: string
  effectiveStatus: string
  dependencyStatus?: string
}

export interface WifiStatusResponse {
  wifiStatus: 'connected' | 'connecting' | 'disconnected' | 'failed' | 'idle' | 'ble_config'
  stationIp: string
  setupApIp: string
  bleProvisioningSupported: boolean
  success?: boolean
  status?: string
}

export interface WifiScanNetwork {
  ssid: string
  rssi: number
  channel: number
}

export interface WifiScanResponse {
  status: 'ok' | 'scanning'
  networks?: WifiScanNetwork[]
  success?: boolean
}

export interface DeviceRecord<
  TConfig extends BaseDeviceConfig = BaseDeviceConfig,
  TRuntime extends BaseDeviceRuntime = BaseDeviceRuntime,
> {
  record: DeviceRecordBase
  config: TConfig
  runtime: TRuntime
  ha?: DeviceHaSettings
}

export interface DeviceCreateRequest<TConfig extends BaseDeviceConfig = BaseDeviceConfig> {
  typeName: string
  config: TConfig
}

export interface DeviceRegistryResponse<TRecord extends DeviceRecord = DeviceRecord> {
  registryRevision: number
  devices: TRecord[]
  success?: boolean
}

export interface DeviceCommandRequest {
  deviceId?: number
  command:
    | 'delete'
    | 'updateConfig'
    | 'scan'
    | 'setOutput'
    | 'setDeps'
    | 'resetDiagnostics'
    | 'checkDevice'
    | 'setHaSettings'
    | 'setMode'
    | 'startDose'
    | 'stopDose'
    | 'setVolume'
    | 'skipNext'
  // 'setOutput' only: boolean/number for switch/analog-style live values, or an object for
  // richer live outputs (e.g. pixel_effect_solid's {r,g,b} color, or the {on} explicit on/off
  // gate shared by pixel_strip/pixel_effect_solid) -- the firmware's REST controller passes an
  // object through verbatim to the device's own handleCommand(), which owns parsing/validating
  // its shape (see docs/pixel-strip.md).
  state?: boolean | number | PixelColor | { on: boolean }
  config?: Record<string, unknown>
  deps?: DeviceDependencyLink[]
  csPin?: number
  haEnabled?: boolean
  haName?: string
  // 'setMode' only - a free-text payload forwarded verbatim to the device runtime's
  // handleCommand() as a DeviceCommandType::Custom command (see AutoSwitchDevice's
  // 'auto'/'pause'/'resume' mode-overlay commands and DosingPumpDevice's 'auto'/'manual').
  mode?: string
  // 'startDose' only: dose volume; logging=false marks a calibration run excluded from
  // totals and the journal (defaults to true).
  amountMl?: number
  logging?: boolean
  // 'setVolume' only: the container's new current volume (refill/correction).
  volumeMl?: number
  // 'skipNext' only: schedule dose slot index + desired one-shot skip flag.
  doseIndex?: number
  skip?: boolean
}

export interface DeviceMutationResponse<TRecord extends DeviceRecord = DeviceRecord> {
  registryRevision: number
  device?: TRecord
  success?: boolean
}

export interface DeviceSetupTransferResponse {
  registryRevision: number
  deviceCount: number
  warnings?: string[]
  success?: boolean
  status?: string
}

export interface DeviceDetailResponse<TRecord extends DeviceRecord = DeviceRecord> {
  registryRevision: number
  device: TRecord
  success?: boolean
}

/** GET /api/devices/:id/layout — the display layout, no longer embedded in the device `config`.
 * With `?page=<index>` only that page is returned in `pages`. */
export interface DeviceLayoutResponse {
  success?: boolean
  schemaVersion: number
  activePageId: string
  pages: Array<Record<string, unknown>>
}

export type MetricNamespace = 'dev' | 'system'

export type MetricValueType = 'null' | 'bool' | 'int' | 'float' | 'string' | 'datetime' | 'duration'

export interface MetricPlaceholderDescriptor {
  placeholder: string
  namespace: MetricNamespace
  sourceId: number
  sourceLabel?: string
  metricId: number
  metricKey: string
  label: string
  valueType: MetricValueType
  available: boolean
  preview?: string
  /** Raw numeric value behind `preview`: epoch seconds ('datetime'), ms ('duration'), or the
   * number itself ('int'/'float'). Lets a client-side `format`/`fixed` filter reformat a value it
   * only ever sees as JSON, without access to firmware's MetricValue layout. */
  previewNumber?: number
}

export interface MetricPlaceholderCatalogResponse {
  registryRevision: number
  placeholders: MetricPlaceholderDescriptor[]
  success?: boolean
}

export interface MetricValueDescriptor {
  namespace: MetricNamespace
  sourceId: number
  sourceLabel?: string
  metricId: number
  metricKey: string
  valueType: MetricValueType
  available: boolean
  value: string
}

export interface MetricValuesResponse {
  registryRevision: number
  values: MetricValueDescriptor[]
  success?: boolean
}

export type DashboardLayoutWidgetRecord = [deviceId: number, x: number, y: number, w: number, h: number]

export interface DashboardLayoutPanelRecord {
  id: string
  name: string
  order: number
  widgets: DashboardLayoutWidgetRecord[]
}

export interface DashboardLayoutRecord {
  schemaVersion: number
  activePanelId: string
  panels: DashboardLayoutPanelRecord[]
}

export interface DashboardLayoutResponse {
  revision: number
  layoutDefaulted?: boolean
  layout: DashboardLayoutRecord
  success?: boolean
}

export interface OtaStatusResponse {
  enabled: boolean
  freeSketchSpace: number
  hasError: boolean
  success?: boolean
  status?: string
}

export interface SystemRestartResponse {
  rebooting: boolean
  success?: boolean
  status?: string
}

export interface SystemVersionResponse {
  version: string
  buildDate: string
  success?: boolean
}

export interface PartitionInfo {
  label: string
  type: string
  subtype: string
  offset: number
  sizeBytes: number
}

export interface FilesystemUsage {
  label: string
  mounted: boolean
  totalBytes: number
  usedBytes: number
}

export interface SystemStatusResponse {
  chip: {
    model: string
    revision: number
    cores: number
    cpuFreqMhz: number
    flashSizeBytes: number
  }
  capabilities: {
    rmtPulseCapture: boolean
  }
  uptimeSeconds: number
  resetReason: string
  heap: {
    totalBytes: number
    freeBytes: number
    minFreeBytes: number
    maxAllocBytes: number
  }
  sketch: {
    usedBytes: number
    partitionBytes: number
  }
  partitions: PartitionInfo[]
  filesystems: FilesystemUsage[]
  nvs: {
    usedEntries: number
    freeEntries: number
    totalEntries: number
    namespaceCount: number
  }
  success?: boolean
  status?: string
}

export interface MqttStatusResponse {
  enabled: boolean
  connected: boolean
  waitingForStation: boolean
  host: string
  port: number
  useTls: boolean
  clientId: string
  hasCaCert: boolean
  success?: boolean
}

export interface MqttSettingsRecord {
  enabled: boolean
  host: string
  port: number
  useTls: boolean
  clientId: string
  username: string
  password: string
  passwordRedacted?: boolean
  haDiscoveryPrefix: string
  haNodeId: string
  haNodeName: string
  hasCaCert?: boolean
  success?: boolean
}

export interface TimeStatusResponse {
  enabled: boolean
  synced: boolean
  waitingForStation: boolean
  ntpServer: string
  timezoneId: string
  syncIntervalSeconds: number
  source: 'ntp' | 'manual' | 'rtc'
  currentEpochUtc?: number
  lastSyncEpochUtc?: number
  localTimeIso8601?: string
  utcOffsetMinutes?: number
  timezoneAbbrev?: string
  success?: boolean
}

export interface TimeSettingsRecord {
  enabled: boolean
  ntpServer: string
  timezoneId: string
  syncIntervalSeconds: number
  success?: boolean
}

export interface SetTimeRequest {
  iso8601: string
}

/** GET/PUT /api/system/board -- selected controller board model (pin-picker UI hints only; pin
 * validity is always checked at the chip level regardless of this selection). */
export interface BoardSettingsResponse {
  chip: string
  selectedBoardId: string
  supportedBoardIds: string[]
  success?: boolean
}

/** GET /api/system/pins -- which GPIO is currently claimed by which device (firmware-authoritative,
 * see docs/gpio-pin-occupancy.md). Only occupied pins are listed; anything absent is free. */
export interface PinOccupancyEntry {
  gpio: number
  deviceId: number
}

export interface PinOccupancyResponse {
  pins: PinOccupancyEntry[]
  success?: boolean
}

/** GET/PUT /api/system/persistence/settings -- device-registry flush debounce/max-delay. */
export interface PersistenceSettingsRecord {
  debounceMs: number
  maxDelayMs: number
  success?: boolean
}

/** POST /api/devices/flush -- forces pending registry persistence to be flushed immediately. */
export interface DeviceFlushResponse {
  success: boolean
  registryRevision: number
  pendingPersistence: boolean
}

export interface TimezoneCatalogEntry {
  id: string
  name: string
}

export interface TimezoneCatalogResponse {
  timezones: TimezoneCatalogEntry[]
  success?: boolean
}

// Response from POST /api/blobs/<prefix> - the server generates the unique part of the key and
// returns the full key; callers never construct it themselves (see docs/blob-store.md).
export interface BlobUploadResponse {
  success: boolean
  key: string
}
