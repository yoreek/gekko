import type {
  DeviceCommandRequest,
  DeviceDetailResponse,
  DeviceLayoutResponse,
  DashboardLayoutRecord,
  DashboardLayoutResponse,
  DeviceCreateRequest,
  DeviceMutationResponse,
  MetricPlaceholderCatalogResponse,
  MetricValuesResponse,
  DeviceSetupTransferResponse,
  DeviceRegistryResponse,
  MqttSettingsRecord,
  MqttStatusResponse,
  OtaStatusResponse,
  SetTimeRequest,
  SystemRestartResponse,
  SystemStatusResponse,
  SystemVersionResponse,
  TimeSettingsRecord,
  TimeStatusResponse,
  TimezoneCatalogResponse,
  WifiScanResponse,
  WifiStatusResponse,
} from './contracts'
import { TIMEZONE_CATALOG } from '@/data/timezones'
import { detectTransportMode } from './transport'
import { requestEmpty, requestFormData, requestJson, requestText } from './http'
import {
  mockCommandDevice,
  mockDeleteMqttCaCert,
  mockExportDeviceSetupBundle,
  mockFetchDashboardLayout,
  mockFetchMqttSettings,
  mockFetchMqttStatus,
  mockImportDeviceSetupBundle,
  mockConfigureWifi,
  mockCreateDevice,
  mockDeleteDevice,
  mockFetchDevice,
  mockFetchDeviceLayout,
  mockFetchDevices,
  mockFetchMetricPlaceholders,
  mockFetchMetricValues,
  mockFetchOtaStatus,
  mockFetchSystemStatus,
  mockFetchSystemVersion,
  mockFetchWifiScan,
  mockFetchWifiStatus,
  mockRestartSystem,
  mockSaveDashboardLayout,
  mockResetWifiCredentials,
  mockStartBleWifiConfig,
  mockFetchTimeSettings,
  mockFetchTimeStatus,
  mockSetSystemTime,
  mockUpdateMqttSettings,
  mockUpdateTimeSettings,
  mockUploadMqttCaCert,
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
  return requestJson<{ status: string }>('/api/wifi/configure', {
    method: 'POST',
    body: JSON.stringify({ ssid, password }),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}

export function startBleWifiConfig(): Promise<{ status: string; action: string }> {
  if (useMockTransport()) {
    return mockStartBleWifiConfig()
  }
  return requestJson<{ status: string; action: string }>('/api/wifi/ble-config', {
    method: 'POST',
    body: JSON.stringify({}),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}

export function resetWifiCredentials(): Promise<{ status: string; action: string }> {
  if (useMockTransport()) {
    return mockResetWifiCredentials()
  }
  return requestJson<{ status: string; action: string }>('/api/wifi/configure', { method: 'DELETE' })
}

export function fetchDevices(): Promise<DeviceRegistryResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchDevices())
  }
  return requestJson<DeviceRegistryResponse>('/api/devices')
}

export function fetchMetricPlaceholders(): Promise<MetricPlaceholderCatalogResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchMetricPlaceholders())
  }
  return requestJson<MetricPlaceholderCatalogResponse>('/api/metrics/placeholders')
}

export function fetchMetricValues(): Promise<MetricValuesResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchMetricValues())
  }
  return requestJson<MetricValuesResponse>('/api/metrics/values')
}

export function fetchDevice(deviceId: number): Promise<DeviceDetailResponse> {
  if (useMockTransport()) {
    return Promise.resolve().then(() => mockFetchDevice(deviceId))
  }
  return requestJson<DeviceDetailResponse>(`/api/devices/${deviceId}`)
}

export function fetchDeviceLayout(deviceId: number, page?: number): Promise<DeviceLayoutResponse> {
  if (useMockTransport()) {
    return Promise.resolve().then(() => mockFetchDeviceLayout(deviceId, page))
  }
  const query = typeof page === 'number' ? `?page=${page}` : ''
  return requestJson<DeviceLayoutResponse>(`/api/devices/${deviceId}/layout${query}`)
}

export function createDevice(payload: DeviceCreateRequest): Promise<DeviceMutationResponse> {
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
    body: JSON.stringify(payload),
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

export function fetchDashboardLayout(): Promise<DashboardLayoutResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchDashboardLayout())
  }
  return requestJson<DashboardLayoutResponse>('/api/dashboard/layout')
}

export function saveDashboardLayout(layout: DashboardLayoutRecord): Promise<void> {
  if (useMockTransport()) {
    return mockSaveDashboardLayout(layout)
  }
  return requestEmpty('/api/dashboard/layout', {
    method: 'PUT',
    body: JSON.stringify({ layout }),
    headers: {
      'Content-Type': 'application/json',
    },
  })
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
  return requestJson<SystemRestartResponse>('/api/system/restart', {
    method: 'POST',
    body: JSON.stringify({}),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}

export function fetchSystemVersion(): Promise<SystemVersionResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchSystemVersion())
  }
  return requestJson<SystemVersionResponse>('/api/system/version')
}

export function fetchSystemStatus(): Promise<SystemStatusResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchSystemStatus())
  }
  return requestJson<SystemStatusResponse>('/api/system/status')
}

export function fetchDeviceSetupBundle(): Promise<string> {
  if (useMockTransport()) {
    return Promise.resolve(mockExportDeviceSetupBundle())
  }
  return requestText('/api/device-setup/export', {
    headers: {
      Accept: 'application/x-ndjson',
    },
  })
}

export function importDeviceSetupBundle(file: File): Promise<DeviceSetupTransferResponse> {
  if (useMockTransport()) {
    return mockImportDeviceSetupBundle(file)
  }

  const formData = new FormData()
  formData.append('bundle', file, file.name || 'device-setup.ndjson')
  return requestFormData<DeviceSetupTransferResponse>('/api/device-setup/import', formData, {
    method: 'POST',
  })
}

export function fetchMqttStatus(): Promise<MqttStatusResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchMqttStatus())
  }
  return requestJson<MqttStatusResponse>('/api/mqtt/status')
}

export function fetchMqttSettings(): Promise<MqttSettingsRecord> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchMqttSettings())
  }
  return requestJson<MqttSettingsRecord>('/api/mqtt/settings')
}

export function updateMqttSettings(settings: Partial<MqttSettingsRecord>): Promise<MqttSettingsRecord> {
  if (useMockTransport()) {
    return mockUpdateMqttSettings(settings)
  }
  return requestJson<MqttSettingsRecord>('/api/mqtt/settings', {
    method: 'PUT',
    body: JSON.stringify(settings),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}

export function uploadMqttCaCert(file: File): Promise<MqttStatusResponse> {
  if (useMockTransport()) {
    return mockUploadMqttCaCert(file)
  }

  const formData = new FormData()
  formData.append('cert', file, file.name || 'ca.pem')
  return requestFormData<MqttStatusResponse>('/api/mqtt/ca-cert', formData, {
    method: 'POST',
  })
}

export function deleteMqttCaCert(): Promise<MqttStatusResponse> {
  if (useMockTransport()) {
    return mockDeleteMqttCaCert()
  }
  return requestJson<MqttStatusResponse>('/api/mqtt/ca-cert', { method: 'DELETE' })
}

export function fetchTimeStatus(): Promise<TimeStatusResponse> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchTimeStatus())
  }
  return requestJson<TimeStatusResponse>('/api/system/time')
}

export function fetchTimeSettings(): Promise<TimeSettingsRecord> {
  if (useMockTransport()) {
    return Promise.resolve(mockFetchTimeSettings())
  }
  return requestJson<TimeSettingsRecord>('/api/system/time/settings')
}

export function updateTimeSettings(settings: Partial<TimeSettingsRecord>): Promise<TimeSettingsRecord> {
  if (useMockTransport()) {
    return mockUpdateTimeSettings(settings)
  }
  return requestJson<TimeSettingsRecord>('/api/system/time/settings', {
    method: 'PUT',
    body: JSON.stringify(settings),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}

export function fetchTimezones(): Promise<TimezoneCatalogResponse> {
  // The firmware no longer serves this list - it's bundled in the SPA (single source of truth).
  // Both mock and live transports resolve the same local catalog.
  return Promise.resolve({ timezones: TIMEZONE_CATALOG, success: true })
}

export function setSystemTime(payload: SetTimeRequest): Promise<TimeStatusResponse> {
  if (useMockTransport()) {
    return mockSetSystemTime(payload)
  }
  return requestJson<TimeStatusResponse>('/api/system/time', {
    method: 'POST',
    body: JSON.stringify(payload),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}
