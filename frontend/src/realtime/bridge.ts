import type { RealtimeMessage } from './messages'
import type { useAppStore } from '@/stores/app'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useOtaStore } from '@/stores/ota'
import { useSystemStore } from '@/stores/system'
import { useWebSocketStore } from '@/stores/websocket'
import { useWifiStore } from '@/stores/wifi'
import type { Pinia } from 'pinia'

type AppStore = ReturnType<typeof useAppStore>

export function bindRealtimeBridge(
  pinia: Pinia,
  appStore: AppStore,
  subscribe: (listener: (message: RealtimeMessage) => void) => () => void,
): () => void {
  const deviceStore = useDeviceRegistryStore(pinia)
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
        const payload = message.payload as { wifi_status?: string }
        if (typeof payload.wifi_status === 'string') {
          appStore.wifiStatus = payload.wifi_status
          appStore.mode = payload.wifi_status === 'ap' || payload.wifi_status === 'ble_config' ? 'ap' : 'station'
        }
        wifiStore.replaceStatus(message.payload as { wifi_status: 'connected' | 'connecting' | 'disconnected' | 'failed' | 'idle'; station_ip: string; setup_ap_ip: string })
        break
      }
      case 'ota.status': {
        const payload = message.payload as { enabled?: boolean }
        if (typeof payload.enabled === 'boolean') {
          appStore.otaEnabled = payload.enabled
        }
        otaStore.replaceFromResponse(message.payload as { enabled: boolean; free_sketch_space: number; has_error: boolean; status?: string })
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
        appStore.registryRevision = message.revision
        deviceStore.setRevision(message.revision)
        const payload = message.payload as { pending_persistence?: boolean }
        if (typeof payload.pending_persistence === 'boolean') {
          deviceStore.setPendingPersistence(payload.pending_persistence)
        }
        break
      }
      case 'device.remove': {
        appStore.registryRevision = message.revision
        deviceStore.setRevision(message.revision)
        const payload = message.payload as { device_id?: number }
        if (typeof payload.device_id === 'number') {
          deviceStore.removeDevice(payload.device_id, message.revision)
        } else {
          deviceStore.setRevision(message.revision)
        }
        if (typeof (message.payload as { pending_persistence?: boolean }).pending_persistence === 'boolean') {
          deviceStore.setPendingPersistence((message.payload as { pending_persistence?: boolean }).pending_persistence === true)
        }
        break
      }
      case 'device.command_result': {
        appStore.registryRevision = message.revision
        deviceStore.setRevision(message.revision)
        const payload = message.payload as { pending_persistence?: boolean }
        if (typeof payload.pending_persistence === 'boolean') {
          deviceStore.setPendingPersistence(payload.pending_persistence)
        }
        break
      }
      default:
        break
    }
  })
}
