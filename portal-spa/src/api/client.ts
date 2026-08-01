import type {
  BoardSettingsResponse,
  PinOccupancyResponse,
  DeviceCommandRequest,
  DeviceDetailResponse,
  DeviceLayoutResponse,
  DashboardLayoutRecord,
  DashboardLayoutResponse,
  DeviceCreateRequest,
  DeviceFlushResponse,
  DeviceMutationResponse,
  MetricPlaceholderCatalogResponse,
  MetricValuesResponse,
  DeviceSetupTransferResponse,
  DeviceRegistryResponse,
  BlobUploadResponse,
  MqttSettingsRecord,
  MqttStatusResponse,
  OtaStatusResponse,
  PersistenceSettingsRecord,
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
import { requestBinary, requestBlob, requestEmpty, requestFormData, requestJson, requestText } from './http'

// `import.meta.env.DEV` is inlined to a literal by Vite's build-time define pass, so in a
// production build (`vite build`, what ships to `data/` and the device's LittleFS) this whole
// branch - and the dynamic import()s it guards - is provably dead code. Rollup drops it (and
// never emits a chunk for `@/mock/handlers` or its dependency tree) instead of shipping the
// mock device fixtures to real hardware. Dev server / Playwright runs (`pnpm dev`) keep DEV
// true, so mock mode keeps working there.
function useMockTransport(): boolean {
  return import.meta.env.DEV && detectTransportMode() === 'mock'
}

export function fetchWifiStatus(): Promise<WifiStatusResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchWifiStatus())
  }
  return requestJson<WifiStatusResponse>('/api/wifi/status')
}

export function fetchWifiScan(): Promise<WifiScanResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchWifiScan())
  }
  return requestJson<WifiScanResponse>('/api/wifi/scan')
}

export function configureWifi(ssid: string, password = ''): Promise<{ status: string }> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockConfigureWifi(ssid, password))
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
    return import('@/mock/handlers').then(m => m.mockStartBleWifiConfig())
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
    return import('@/mock/handlers').then(m => m.mockResetWifiCredentials())
  }
  return requestJson<{ status: string; action: string }>('/api/wifi/configure', { method: 'DELETE' })
}

export function fetchDevices(): Promise<DeviceRegistryResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchDevices())
  }
  return requestJson<DeviceRegistryResponse>('/api/devices')
}

export function fetchMetricPlaceholders(): Promise<MetricPlaceholderCatalogResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchMetricPlaceholders())
  }
  return requestJson<MetricPlaceholderCatalogResponse>('/api/metrics/placeholders')
}

export function fetchMetricValues(): Promise<MetricValuesResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchMetricValues())
  }
  return requestJson<MetricValuesResponse>('/api/metrics/values')
}

export function fetchDevice(deviceId: number): Promise<DeviceDetailResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchDevice(deviceId))
  }
  return requestJson<DeviceDetailResponse>(`/api/devices/${deviceId}`)
}

export function fetchDeviceLayout(deviceId: number, page?: number): Promise<DeviceLayoutResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchDeviceLayout(deviceId, page))
  }
  const query = typeof page === 'number' ? `?page=${page}` : ''
  return requestJson<DeviceLayoutResponse>(`/api/devices/${deviceId}/layout${query}`)
}

export function createDevice(payload: DeviceCreateRequest): Promise<DeviceMutationResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockCreateDevice(payload))
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
    return import('@/mock/handlers').then(m => m.mockCommandDevice(deviceId, payload))
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
    return import('@/mock/handlers').then(m => m.mockDeleteDevice(deviceId))
  }
  return requestJson<DeviceMutationResponse>(`/api/devices/${deviceId}`, { method: 'DELETE' })
}

export function fetchDashboardLayout(): Promise<DashboardLayoutResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchDashboardLayout())
  }
  return requestJson<DashboardLayoutResponse>('/api/dashboard/layout')
}

export function saveDashboardLayout(layout: DashboardLayoutRecord): Promise<void> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockSaveDashboardLayout(layout))
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
    return import('@/mock/handlers').then(m => m.mockFetchOtaStatus())
  }
  return requestJson<OtaStatusResponse>('/api/ota/status')
}

export function restartSystem(): Promise<SystemRestartResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockRestartSystem())
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
    return import('@/mock/handlers').then(m => m.mockFetchSystemVersion())
  }
  return requestJson<SystemVersionResponse>('/api/system/version')
}

export function fetchSystemStatus(): Promise<SystemStatusResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchSystemStatus())
  }
  return requestJson<SystemStatusResponse>('/api/system/status')
}

export function fetchDeviceSetupBundle(): Promise<string> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockExportDeviceSetupBundle())
  }
  return requestText('/api/device-setup/export', {
    headers: {
      Accept: 'application/x-ndjson',
    },
  })
}

export function importDeviceSetupBundle(file: File): Promise<DeviceSetupTransferResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockImportDeviceSetupBundle(file))
  }

  const formData = new FormData()
  formData.append('bundle', file, file.name || 'device-setup.ndjson')
  return requestFormData<DeviceSetupTransferResponse>('/api/device-setup/import', formData, {
    method: 'POST',
  })
}

export function fetchMqttStatus(): Promise<MqttStatusResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchMqttStatus())
  }
  return requestJson<MqttStatusResponse>('/api/mqtt/status')
}

export function fetchMqttSettings(): Promise<MqttSettingsRecord> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchMqttSettings())
  }
  return requestJson<MqttSettingsRecord>('/api/mqtt/settings')
}

export function updateMqttSettings(settings: Partial<MqttSettingsRecord>): Promise<MqttSettingsRecord> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockUpdateMqttSettings(settings))
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
    return import('@/mock/handlers').then(m => m.mockUploadMqttCaCert(file))
  }

  const formData = new FormData()
  formData.append('cert', file, file.name || 'ca.pem')
  return requestFormData<MqttStatusResponse>('/api/mqtt/ca-cert', formData, {
    method: 'POST',
  })
}

export function deleteMqttCaCert(): Promise<MqttStatusResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockDeleteMqttCaCert())
  }
  return requestJson<MqttStatusResponse>('/api/mqtt/ca-cert', { method: 'DELETE' })
}

export function fetchTimeStatus(): Promise<TimeStatusResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchTimeStatus())
  }
  return requestJson<TimeStatusResponse>('/api/system/time')
}

export function fetchTimeSettings(): Promise<TimeSettingsRecord> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchTimeSettings())
  }
  return requestJson<TimeSettingsRecord>('/api/system/time/settings')
}

export function updateTimeSettings(settings: Partial<TimeSettingsRecord>): Promise<TimeSettingsRecord> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockUpdateTimeSettings(settings))
  }
  return requestJson<TimeSettingsRecord>('/api/system/time/settings', {
    method: 'PUT',
    body: JSON.stringify(settings),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}

export function fetchBoardSettings(): Promise<BoardSettingsResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchBoardSettings())
  }
  return requestJson<BoardSettingsResponse>('/api/system/board')
}

export function updateBoardSettings(boardId: string): Promise<BoardSettingsResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockUpdateBoardSettings(boardId))
  }
  return requestJson<BoardSettingsResponse>('/api/system/board', {
    method: 'PUT',
    body: JSON.stringify({ boardId }),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}

export function fetchPinOccupancy(): Promise<PinOccupancyResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchPinOccupancy())
  }
  return requestJson<PinOccupancyResponse>('/api/system/pins')
}

export function fetchPersistenceSettings(): Promise<PersistenceSettingsRecord> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchPersistenceSettings())
  }
  return requestJson<PersistenceSettingsRecord>('/api/system/persistence/settings')
}

export function updatePersistenceSettings(settings: Partial<PersistenceSettingsRecord>): Promise<PersistenceSettingsRecord> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockUpdatePersistenceSettings(settings))
  }
  return requestJson<PersistenceSettingsRecord>('/api/system/persistence/settings', {
    method: 'PUT',
    body: JSON.stringify(settings),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}

export function flushDevicePersistence(): Promise<DeviceFlushResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFlushDevicePersistence())
  }
  return requestJson<DeviceFlushResponse>('/api/devices/flush', { method: 'POST' })
}

export function fetchTimezones(): Promise<TimezoneCatalogResponse> {
  // The firmware no longer serves this list - it's bundled in the SPA (single source of truth).
  // Both mock and live transports resolve the same local catalog.
  return Promise.resolve({ timezones: TIMEZONE_CATALOG, success: true })
}

export function setSystemTime(payload: SetTimeRequest): Promise<TimeStatusResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockSetSystemTime(payload))
  }
  return requestJson<TimeStatusResponse>('/api/system/time', {
    method: 'POST',
    body: JSON.stringify(payload),
    headers: {
      'Content-Type': 'application/json',
    },
  })
}

// Generic blob store (docs/blob-store.md). `prefix` is a grouping label the caller already knows
// (e.g. "dev/<deviceId hex>") - the server generates the unique part of the key and returns the
// full key in the response; the caller must never construct the key itself.
export function uploadBlob(prefix: string, bytes: Blob | ArrayBuffer): Promise<BlobUploadResponse> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockUploadBlob(prefix, bytes))
  }
  return requestBinary<BlobUploadResponse>(`/api/blobs/${prefix}`, bytes, {
    method: 'POST',
    headers: { 'Content-Type': 'application/octet-stream' },
  })
}

export function fetchBlob(key: string): Promise<Blob> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockFetchBlob(key))
  }
  return requestBlob(`/api/blobs/${key}`)
}

export function deleteBlob(key: string): Promise<void> {
  if (useMockTransport()) {
    return import('@/mock/handlers').then(m => m.mockDeleteBlob(key))
  }
  return requestEmpty(`/api/blobs/${key}`, { method: 'DELETE' })
}
