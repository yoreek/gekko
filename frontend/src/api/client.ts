import type {
  DeviceCommandRequest,
  DeviceMutationResponse,
  DeviceRegistryResponse,
  OtaStatusResponse,
  SystemRestartResponse,
  WifiScanResponse,
  WifiStatusResponse,
} from './contracts'
import { detectTransportMode } from './transport'
import { requestForm, requestJson } from './http'
import {
  mockCommandDevice,
  mockConfigureWifi,
  mockCreateDevice,
  mockDeleteDevice,
  mockFetchDevices,
  mockFetchOtaStatus,
  mockFetchWifiScan,
  mockFetchWifiStatus,
  mockRestartSystem,
  mockStartBleWifiConfig,
} from '@/mock/handlers'

function useMockTransport(): boolean {
  return detectTransportMode() === 'mock'
}

export function fetchWifiStatus(): Promise<WifiStatusResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchWifiStatus())
  }
  return requestJson<WifiStatusResponse>('/api/wifi/status')
}

export function fetchWifiScan(): Promise<WifiScanResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchWifiScan())
  }
  return requestJson<WifiScanResponse>('/api/wifi/scan')
}

export function configureWifi(ssid: string, password = ''): Promise<{ status: string }> {
  if (useMockTransport()) {
    return mockConfigureWifi(ssid, password)
  }
  const form = new URLSearchParams()
  form.set('ssid', ssid)
  if (password.length > 0) {
    form.set('password', password)
  }
  return requestForm<{ status: string }>('/api/wifi/configure', form)
}

export function startBleWifiConfig(): Promise<{ status: string; action: string }> {
  if (useMockTransport()) {
    return mockStartBleWifiConfig()
  }
  return requestJson<{ status: string; action: string }>('/api/wifi/ble-config', { method: 'POST' })
}

export function fetchDevices(): Promise<DeviceRegistryResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchDevices())
  }
  return requestJson<DeviceRegistryResponse>('/api/devices')
}

export function createDevice(payload: Record<string, unknown>): Promise<DeviceMutationResponse> {
  if (useMockTransport()) {
    return mockCreateDevice(payload)
  }
  return requestJson<DeviceMutationResponse>('/api/devices', {
    method: 'POST',
    body: JSON.stringify(payload),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}

export function commandDevice(deviceId: number, payload: DeviceCommandRequest): Promise<DeviceMutationResponse> {
  if (useMockTransport()) {
    return mockCommandDevice(deviceId, payload)
  }
  return requestJson<DeviceMutationResponse>(`/api/devices/${deviceId}/command`, {
    method: 'POST',
    body: JSON.stringify({ ...payload, device_id: deviceId }),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}

export function deleteDevice(deviceId: number): Promise<DeviceMutationResponse> {
  if (useMockTransport()) {
    return mockDeleteDevice(deviceId)
  }
  return requestJson<DeviceMutationResponse>(`/api/devices/${deviceId}`, { method: 'DELETE' })
}

export function fetchOtaStatus(): Promise<OtaStatusResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchOtaStatus())
  }
  return requestJson<OtaStatusResponse>('/api/ota/status')
}

export function restartSystem(): Promise<SystemRestartResponse> {
  if (useMockTransport()) {
    return mockRestartSystem()
  }
  return requestJson<SystemRestartResponse>('/api/system/restart', { method: 'POST' })
}
