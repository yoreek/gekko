import type { DeviceRecord } from '@/api'
import type { Pinia } from 'pinia'

import { publishRealtimeMessage } from './bus'
import type { RealtimeMessage } from './messages'
import { publishMockSnapshot } from '@/mock/snapshot'
import { loadMockDatabase, saveMockDatabase } from '@/mock/database'
import { useAppStore } from '@/stores/app'
import { useWebSocketStore } from '@/stores/websocket'

export interface MockRealtimeSocketHandle {
  dispose(): void
  publish(message: RealtimeMessage): void
  upsertDevice(device: DeviceRecord): void
  removeDevice(deviceId: number): void
  refreshSnapshot(): void
}

declare global {
  interface Window {
    __gekkoMockRealtime?: MockRealtimeSocketHandle
  }
}

function publishDeviceUpsert(device: DeviceRecord): void {
  publishRealtimeMessage({
    topic: 'device.upsert',
    revision: device.registry_revision ?? loadMockDatabase().registryRevision,
    payload: device,
  })
}

function publishDeviceRemove(deviceId: number, revision: number, pendingPersistence: boolean): void {
  publishRealtimeMessage({
    topic: 'device.remove',
    revision,
    payload: {
      device_id: deviceId,
      registry_revision: revision,
      pending_persistence: pendingPersistence,
    },
  })
}

export function connectMockRealtimeSocket(pinia: Pinia): MockRealtimeSocketHandle {
  const appStore = useAppStore(pinia)
  const wsStore = useWebSocketStore(pinia)

  const markConnected = (): void => {
    wsStore.markConnected()
    appStore.setWebSocketStatus('connected')
  }

  const markDisconnected = (): void => {
    wsStore.markDisconnected()
    appStore.setWebSocketStatus('disconnected')
  }

  const handle: MockRealtimeSocketHandle = {
    dispose(): void {
      if (typeof window !== 'undefined' && window.__gekkoMockRealtime === handle) {
        window.__gekkoMockRealtime = undefined
      }
      markDisconnected()
    },
    publish(message: RealtimeMessage): void {
      publishRealtimeMessage(message)
    },
    upsertDevice(device: DeviceRecord): void {
      const db = loadMockDatabase()
      const index = db.devices.findIndex(entry => entry.device_id === device.device_id)
      if (index >= 0) {
        db.devices.splice(index, 1, device)
      } else {
        db.devices.push(device)
      }
      db.registryRevision += 1
      db.pendingPersistence = true
      saveMockDatabase(db)
      publishDeviceUpsert({
        ...device,
        registry_revision: db.registryRevision,
        pending_persistence: db.pendingPersistence,
      })
    },
    removeDevice(deviceId: number): void {
      const db = loadMockDatabase()
      db.devices = db.devices.filter(entry => entry.device_id !== deviceId)
      db.registryRevision += 1
      db.pendingPersistence = true
      saveMockDatabase(db)
      publishDeviceRemove(deviceId, db.registryRevision, db.pendingPersistence)
    },
    refreshSnapshot(): void {
      publishMockSnapshot()
    },
  }

  if (typeof window !== 'undefined') {
    window.__gekkoMockRealtime = handle
  }

  publishMockSnapshot()
  markConnected()

  return handle
}
