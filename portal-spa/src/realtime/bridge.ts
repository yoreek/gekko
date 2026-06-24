import type { RealtimeMessage } from './messages'
import type { useAppStore } from '@/stores/app'
import type { DeviceRecord } from '@/api'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useDeviceEventLogStore } from '@/stores/deviceEventLog'
import { useOtaStore } from '@/stores/ota'
import { useSystemStore } from '@/stores/system'
import { useWebSocketStore } from '@/stores/websocket'
import { useWifiStore } from '@/stores/wifi'
import type { Pinia } from 'pinia'

type AppStore = ReturnType<typeof useAppStore>

function isDeviceRecord(value: unknown): value is DeviceRecord {
  const record = typeof value === 'object' && value !== null ? (value as { record?: unknown }).record : null
  return (
    typeof value === 'object' &&
    value !== null &&
    typeof record === 'object' &&
    record !== null &&
    typeof (record as { id?: unknown }).id === 'number' &&
    typeof (record as { typeName?: unknown }).typeName === 'string' &&
    typeof (record as { configRevision?: unknown }).configRevision === 'number'
  )
}

function extractDeviceRecord(payload: unknown): DeviceRecord | null {
  if (isDeviceRecord(payload)) {
    return payload
  }

  if (typeof payload === 'object' && payload !== null && isDeviceRecord((payload as { device?: unknown }).device)) {
    return (payload as { device: DeviceRecord }).device
  }

  return null
}

export function bindRealtimeBridge(
  pinia: Pinia,
  appStore: AppStore,
  subscribe: (listener: (message: RealtimeMessage) => void) => () => void,
): () => void {
  const deviceStore = useDeviceRegistryStore(pinia)
  const journalStore = useDeviceEventLogStore(pinia)
  const wifiStore = useWifiStore(pinia)
  const otaStore = useOtaStore(pinia)
  const systemStore = useSystemStore(pinia)
  const wsStore = useWebSocketStore(pinia)

  return subscribe(message => {
    wsStore.applyMessage(message)
    switch (message.topic) {
      case 'hello': {
        appStore.registryRevision = message.revision
        appStore.wsStatus = 'connected'
        break
      }
      case 'wifi.status': {
        const payload = message.payload as { wifiStatus?: string }
        const wifiStatus = typeof payload.wifiStatus === 'string' ? payload.wifiStatus : undefined
        if (typeof wifiStatus === 'string') {
          appStore.wifiStatus = wifiStatus
          appStore.mode = wifiStatus === 'ap' || wifiStatus === 'ble_config' ? 'ap' : 'station'
        }
        wifiStore.replaceStatus(message.payload as { wifiStatus: 'connected' | 'connecting' | 'disconnected' | 'failed' | 'idle'; stationIp: string; setupApIp: string })
        break
      }
      case 'ota.status': {
        const payload = message.payload as { enabled?: boolean }
        if (typeof payload.enabled === 'boolean') {
          appStore.otaEnabled = payload.enabled
        }
        otaStore.replaceFromResponse(message.payload as { enabled: boolean; freeSketchSpace: number; hasError: boolean; status?: string })
        break
      }
      case 'system.status': {
        const payload = message.payload as { status?: string; rebooting?: boolean }
        if (typeof payload.status === 'string') {
          appStore.systemStatus = payload.status
        }
        if (typeof payload.rebooting === 'boolean' && payload.rebooting) {
          appStore.systemStatus = 'rebooting'
        }
        systemStore.replaceFromResponse(payload)
        break
      }
      case 'device.upsert': {
        journalStore.append(message)
        appStore.registryRevision = message.revision
        deviceStore.setRevision(message.revision)
        const devicePayload = extractDeviceRecord(message.payload)
        if (isDeviceRecord(devicePayload)) {
          deviceStore.upsertDevice(devicePayload, message.revision)
        }
        break
      }
      case 'device.remove': {
        journalStore.append(message)
        appStore.registryRevision = message.revision
        deviceStore.setRevision(message.revision)
        const payload = message.payload as { deviceId?: number }
        if (typeof payload.deviceId === 'number') {
          deviceStore.removeDevice(payload.deviceId, message.revision)
        } else {
          deviceStore.setRevision(message.revision)
        }
        break
      }
      case 'device.command_result': {
        journalStore.append(message)
        appStore.registryRevision = message.revision
        deviceStore.setRevision(message.revision)
        const devicePayload = extractDeviceRecord(message.payload)
        if (isDeviceRecord(devicePayload)) {
          deviceStore.upsertDevice(devicePayload, message.revision)
        }
        break
      }
      default:
        break
    }
  })
}
