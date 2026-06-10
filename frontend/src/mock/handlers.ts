import type {
  DeviceCommandRequest,
  DeviceDetailResponse,
  DeviceMutationResponse,
  DeviceRecord,
  DeviceRegistryResponse,
  OtaStatusResponse,
  SystemRestartResponse,
  WifiScanResponse,
  WifiStatusResponse,
} from '@/api'
import { ApiClientError } from '@/api/http'
import { publishRealtimeMessage } from '@/realtime/bus'
import { DUMMY_DEVICE_TYPE_ID } from '@/models/device-types'
import { createSeedMockDatabase, loadMockDatabase, saveMockDatabase } from './database'

function ok<T extends object>(payload: T): T & { success: true } {
  return {
    success: true,
    ...payload,
  }
}

function mutateRegistry<T>(mutator: (db: ReturnType<typeof createSeedMockDatabase>) => T): T {
  const db = loadMockDatabase()
  const result = mutator(db)
  saveMockDatabase(db)
  return result
}

export function mockFetchWifiStatus(): WifiStatusResponse {
  const db = loadMockDatabase()
  return ok({
    wifi_status: db.wifi.status,
    station_ip: db.wifi.stationIp,
    setup_ap_ip: db.wifi.setupApIp,
  })
}

export function mockFetchWifiScan(): WifiScanResponse {
  const db = loadMockDatabase()
  return ok({
    status: 'ok',
    networks: db.wifi.scan,
  })
}

export function mockConfigureWifi(ssid: string, password = ''): Promise<{ status: string }> {
  const response = mutateRegistry(db => {
      db.wifi.status = 'connecting'
      db.registryRevision += 1
      db.pendingPersistence = true
      return ok({
        status: 'accepted',
        ssid,
        password,
      })
  })
  const db = loadMockDatabase()
  publishRealtimeMessage({
    topic: 'wifi.status',
    revision: db.registryRevision,
    payload: {
      wifi_status: db.wifi.status,
      station_ip: db.wifi.stationIp,
      setup_ap_ip: db.wifi.setupApIp,
    },
  })
  return Promise.resolve(response)
}

export function mockStartBleWifiConfig(): Promise<{ status: string; action: string }> {
  const response = mutateRegistry(db => {
    db.wifi.status = 'ble_config'
    db.registryRevision += 1
    db.pendingPersistence = true
    return ok({
      status: 'accepted',
      action: 'start_ble_config',
    })
  })
  publishRealtimeMessage({
    topic: 'wifi.status',
    revision: loadMockDatabase().registryRevision,
    payload: {
      wifi_status: 'ble_config',
      station_ip: loadMockDatabase().wifi.stationIp,
      setup_ap_ip: loadMockDatabase().wifi.setupApIp,
    },
  })
  return Promise.resolve(response)
}

export function mockFetchDevices(): DeviceRegistryResponse {
  const db = loadMockDatabase()
  return ok({
    registry_revision: db.registryRevision,
    pending_persistence: db.pendingPersistence,
    devices: db.devices,
  })
}

export function mockFetchDevice(deviceId: number): DeviceDetailResponse {
  const db = loadMockDatabase()
  const device = db.devices.find(entry => entry.device_id === deviceId)
  if (!device) {
    throw new ApiClientError('device not found', 'NOT_FOUND', 404, null)
  }
  return ok({
    registry_revision: db.registryRevision,
    pending_persistence: db.pendingPersistence,
    device,
  })
}

export function mockCreateDevice(payload: Record<string, unknown>): Promise<DeviceMutationResponse> {
  const response = mutateRegistry(db => {
    const nextId = Math.max(1, ...db.devices.map(device => device.device_id)) + 1
    const typeId = payload.type_id
    if (typeof typeId !== 'number' || typeId !== DUMMY_DEVICE_TYPE_ID) {
      throw new ApiClientError('unsupported device type', 'UNSUPPORTED_TYPE', 400, null)
    }

    const device: DeviceRecord = {
      device_id: nextId,
      type_id: typeId,
      label: 'Dummy device',
      type: 'dummy',
      name: String(payload.name ?? 'New Device'),
      enabled: Boolean(payload.enabled ?? true),
      has_parent: false,
      parent_device_id: 0,
      config_version: 1,
      config_revision: 1,
      lifecycle_status: 'ready',
      effective_status: 'ready',
      status: 'ready',
    }
    if (isRecordPayload(payload.config)) {
      device.config = payload.config
    }
    db.devices.push(device)
    db.registryRevision += 1
    db.pendingPersistence = true
    return ok({
      registry_revision: db.registryRevision,
      pending_persistence: db.pendingPersistence,
      device: db.devices.at(-1),
    })
  })
  const db = loadMockDatabase()
  publishRealtimeMessage({
    topic: 'device.upsert',
    revision: db.registryRevision,
    payload: response.device ?? {},
  })
  return Promise.resolve(response)
}

export function mockCommandDevice(deviceId: number, payload: DeviceCommandRequest): Promise<DeviceMutationResponse> {
  const response = mutateRegistry(db => {
      const device = db.devices.find(entry => entry.device_id === deviceId)
      if (!device) {
        throw new ApiClientError('device not found', 'NOT_FOUND', 404, null)
      }

      switch (payload.command) {
        case 'rename':
          device.name = String(payload.payload ?? device.name)
          break
        case 'enable':
          device.enabled = true
          device.lifecycle_status = 'ready'
          device.effective_status = 'ready'
          device.status = 'ready'
          break
        case 'disable':
          device.enabled = false
          device.lifecycle_status = 'disabled'
          device.effective_status = 'disabled'
          device.status = 'disabled'
          break
        case 'delete':
          db.devices = db.devices.filter(entry => entry.device_id !== deviceId)
          break
        case 'update_config':
          device.config_revision += 1
          break
        case 'set_status':
          if (payload.payload === 'fault') {
            device.lifecycle_status = 'faulted'
            device.effective_status = 'faulted'
            device.status = 'faulted'
          }
          if (payload.payload === 'ready') {
            device.lifecycle_status = 'ready'
            device.effective_status = 'ready'
            device.status = 'ready'
          }
          break
        case 'custom':
          if (!isRecordPayload(device.config)) {
            device.config = {}
          }
          if (payload.payload === 'output=1') {
            device.config = {
              ...device.config,
              current_output: true,
            }
          }
          if (payload.payload === 'output=0') {
            device.config = {
              ...device.config,
              current_output: false,
            }
          }
          break
        case 'set_parent':
          device.has_parent = Boolean(payload.has_parent ?? true)
          device.parent_device_id = Number(payload.parent_device_id ?? 0)
          break
        default:
          break
      }

      db.registryRevision += 1
      db.pendingPersistence = true
      return ok({
        registry_revision: db.registryRevision,
        pending_persistence: db.pendingPersistence,
        device: db.devices.find(entry => entry.device_id === deviceId),
      })
  })
  const db = loadMockDatabase()
  publishRealtimeMessage({
    topic: payload.command === 'delete' ? 'device.remove' : 'device.command_result',
    revision: db.registryRevision,
    payload: response.device ?? { device_id: deviceId },
  })
  return Promise.resolve(response)
}

export function mockDeleteDevice(deviceId: number): Promise<DeviceMutationResponse> {
  return mockCommandDevice(deviceId, {
    command: 'delete',
  })
}

export function mockFetchOtaStatus(): OtaStatusResponse {
  const db = loadMockDatabase()
  return ok(db.ota)
}

export function mockRestartSystem(): Promise<SystemRestartResponse> {
  const response = mutateRegistry(db => {
      db.system.status = 'rebooting'
      db.system.rebooting = true
      return ok({
        rebooting: true,
      })
  })
  publishRealtimeMessage({
    topic: 'system.status',
    revision: loadMockDatabase().registryRevision,
    payload: {
      status: 'rebooting',
      rebooting: true,
    },
  })
  return Promise.resolve(response)
}

function isRecordPayload(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}
