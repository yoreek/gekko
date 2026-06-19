export type DeviceOutputState = 'off' | 'on' | 'disabled'

export interface DeviceOutputSnapshot {
  state?: DeviceOutputState
  temperature?: TemperatureOutputSnapshot
}

export type TemperatureUnit = 'celsius' | 'fahrenheit'

export interface TemperatureOutputSnapshot {
  value: number
  unit: TemperatureUnit
  unit_symbol: string
  measured_at_ms: number
  valid: boolean
  status?: string
}

export interface OneWireScanDeviceSnapshot {
  address: string
  family_code: string
}

export interface OneWireScanSnapshot {
  in_progress: boolean
  ready: boolean
  device_count: number
  truncated: boolean
  invalid_crc_seen: boolean
  devices: OneWireScanDeviceSnapshot[]
}

export interface WifiStatusResponse {
  wifi_status: 'connected' | 'connecting' | 'disconnected' | 'failed' | 'idle' | 'ble_config'
  station_ip: string
  setup_ap_ip: string
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

export interface DeviceRecord {
  device_id: number
  type_id: number
  label?: string
  type?: string
  name: string
  enabled: boolean
  has_parent: boolean
  parent_device_id: number
  config_version: number
  config_revision: number
  lifecycle_status: string
  effective_status: string
  status?: string
  config?: Record<string, unknown>
  output?: DeviceOutputSnapshot
  retained_state_supported?: boolean
  retained_startup_enabled?: boolean
  retained_startup_fallback_output?: boolean
  retained_state_in_config_payload?: boolean
  scan?: OneWireScanSnapshot
  registry_revision?: number
  pending_persistence?: boolean
}

export interface DeviceRegistryResponse {
  registry_revision: number
  pending_persistence: boolean
  devices: DeviceRecord[]
  success?: boolean
}

export interface DeviceCommandRequest {
  device_id?: number
  command: 'rename' | 'enable' | 'disable' | 'delete' | 'update_config' | 'set_status' | 'scan' | 'set_output' | 'set_parent'
  name?: string
  status?: string
  state?: DeviceOutputState
  config?: Record<string, unknown>
  has_parent?: boolean
  parent_device_id?: number
}

export interface DeviceMutationResponse {
  registry_revision: number
  pending_persistence: boolean
  device?: DeviceRecord
  success?: boolean
}

export interface DeviceDetailResponse {
  registry_revision: number
  pending_persistence: boolean
  device: DeviceRecord
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
  schema_version: number
  active_panel_id: string
  panels: DashboardLayoutPanelRecord[]
}

export interface DashboardLayoutResponse {
  revision: number
  layout_defaulted?: boolean
  layout: DashboardLayoutRecord
  success?: boolean
}

export interface OtaStatusResponse {
  enabled: boolean
  free_sketch_space: number
  has_error: boolean
  success?: boolean
  status?: string
}

export interface SystemRestartResponse {
  rebooting: boolean
  success?: boolean
  status?: string
}
