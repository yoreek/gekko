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
  name: string
  enabled: boolean
  has_parent: boolean
  parent_device_id: number
  config_version: number
  config_revision: number
  lifecycle_status: string
  effective_status: string
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
  command: 'rename' | 'enable' | 'disable' | 'delete' | 'set_config' | 'set_parent'
  payload?: string
  has_parent?: boolean
  parent_device_id?: number
  persistence_policy?: 'immediate' | 'delayed' | 'coalesced'
}

export interface DeviceMutationResponse {
  registry_revision: number
  pending_persistence: boolean
  device?: DeviceRecord
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
