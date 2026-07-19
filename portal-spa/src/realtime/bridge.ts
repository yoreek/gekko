import type { DeviceRemoveEventPayload, RealtimeMessage } from './messages'
import type { useAppStore } from '@/stores/app'
import type { DeviceRecord } from '@/api'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useDeviceEventLogStore } from '@/stores/deviceEventLog'
import { useDeviceHistoryStore } from '@/stores/deviceHistory'
import { useMqttStore } from '@/stores/mqtt'
import { useOtaStore } from '@/stores/ota'
import { useSystemStore } from '@/stores/system'
import { useTimeStore } from '@/stores/time'
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
  const journalStore = useDeviceEventLogStore(pinia)
  const historyStore = useDeviceHistoryStore(pinia)
  const wifiStore = useWifiStore(pinia)
  const otaStore = useOtaStore(pinia)
  const systemStore = useSystemStore(pinia)
  const mqttStore = useMqttStore(pinia)
  const timeStore = useTimeStore(pinia)
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
      case 'mqtt.status': {
        const payload = message.payload as { enabled?: boolean; connected?: boolean; waitingForStation?: boolean }
        mqttStore.replaceFromStatus({
          enabled: Boolean(payload.enabled),
          connected: Boolean(payload.connected),
          waitingForStation: Boolean(payload.waitingForStation),
          host: mqttStore.host,
          port: mqttStore.port,
          useTls: mqttStore.useTls,
          clientId: mqttStore.clientId,
          hasCaCert: mqttStore.hasCaCert,
        })
        break
      }
      case 'time.status': {
        const payload = message.payload as {
          synced?: boolean
          currentEpochUtc?: number
          timezoneId?: string
          source?: 'ntp' | 'manual' | 'rtc'
        }
        timeStore.replaceFromStatus({
          enabled: timeStore.enabled,
          synced: Boolean(payload.synced),
          waitingForStation: timeStore.waitingForStation,
          ntpServer: timeStore.ntpServer,
          timezoneId: payload.timezoneId ?? timeStore.timezoneId,
          syncIntervalSeconds: timeStore.syncIntervalSeconds,
          source: payload.source ?? timeStore.source,
          currentEpochUtc: payload.currentEpochUtc,
        })
        break
      }
      case 'device.upsert': {
        journalStore.append(message)
        historyStore.recordSample(message.payload as DeviceRecord)
        appStore.registryRevision = message.revision
        deviceStore.setRevision(message.revision)
        deviceStore.upsertDevice(message.payload as DeviceRecord, message.revision)
        break
      }
      case 'device.remove': {
        journalStore.append(message)
        appStore.registryRevision = message.revision
        deviceStore.setRevision(message.revision)
        deviceStore.removeDevice((message.payload as DeviceRemoveEventPayload).deviceId, message.revision)
        break
      }
      case 'device.command_result': {
        journalStore.append(message)
        historyStore.recordSample(message.payload as DeviceRecord)
        appStore.registryRevision = message.revision
        deviceStore.setRevision(message.revision)
        deviceStore.upsertDevice(message.payload as DeviceRecord, message.revision)
        break
      }
      default:
        break
    }
  })
}
