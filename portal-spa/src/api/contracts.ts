export type DeviceOutputState = 'off' | 'on' | 'disabled'

export interface GpioSwitchOutputSnapshot {
  state?: DeviceOutputState
  physicalLevel?: boolean
}

export interface Ds18b20TemperatureSensorOutputSnapshot {
  temperature?: TemperatureOutputSnapshot
}

export interface ThermostatOutputSnapshot {
  desiredSwitchState?: DeviceOutputState
  actualSwitchState?: DeviceOutputState
  controlStatus?: string
  lastCheckAtMs?: number
  temperature?: TemperatureOutputSnapshot
}

export type DeviceOutputSnapshot =
  | GpioSwitchOutputSnapshot
  | Ds18b20TemperatureSensorOutputSnapshot
  | ThermostatOutputSnapshot

export type TemperatureUnit = 'celsius' | 'fahrenheit'

export interface TemperatureOutputSnapshot {
  value: number
  unit: TemperatureUnit
  unitSymbol: string
  measuredAtMs: number
  valid: boolean
  status?: string
}

export interface OneWireScanDeviceSnapshot {
  address: string
  familyCode: string
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
  role: string
  deviceId: number
}

export interface DeviceRecordBase {
  id: number
  typeName: string
  configRevision: number
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
  state?: DeviceOutputState
  config?: Record<string, unknown>
  deps?: DeviceDependencyLink[]
  csPin?: number
}

export interface DeviceMutationResponse<TRecord extends DeviceRecord = DeviceRecord> {
  registryRevision: number
  device?: TRecord
  success?: boolean
}

export interface DeviceSetupTransferResponse {
  registryRevision: number
  deviceCount: number
  success?: boolean
  status?: string
}

export interface DeviceDetailResponse<TRecord extends DeviceRecord = DeviceRecord> {
  registryRevision: number
  device: TRecord
  success?: boolean
}

export type MetricNamespace = 'dev' | 'system'

export type MetricValueType = 'null' | 'bool' | 'int' | 'float' | 'string'

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
