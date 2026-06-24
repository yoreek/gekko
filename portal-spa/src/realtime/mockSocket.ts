import type { Pinia } from 'pinia'

import { publishRealtimeMessage } from './bus'
import type { RealtimeMessage } from './messages'
import { publishMockSnapshot } from '@/mock/snapshot'
import { loadMockDatabase, saveMockDatabase } from '@/mock/database'
import { decorateDeviceRecord, publishThermostatDependents, refreshMockDerivedDeviceState } from '@/mock/handlers'
import { useAppStore } from '@/stores/app'
import { useWebSocketStore } from '@/stores/websocket'

type DeviceRecord = Record<string, any>

export interface MockRealtimeSocketHandle {
  dispose(): void
  publish(message: RealtimeMessage): void
  upsertDevice(device: DeviceRecord): void
  removeDevice(deviceId: number): void
  refreshSnapshot(): void
  schedulePersistenceFlush(): void
}

declare global {
  interface Window {
    __gekkoMockRealtime?: MockRealtimeSocketHandle
    __gekkoMockRealtimeRuntime?: {
      schedulePersistenceFlush?: () => void
    }
  }
}

function publishDeviceUpsert(device: DeviceRecord, eventKind: 'device_created' | 'device_updated' | 'snapshot' = 'device_updated'): void {
  const db = loadMockDatabase()
  const payload = decorateDeviceRecord(device, device.registryRevision ?? db.registryRevision)
  publishRealtimeMessage({
    topic: 'device.upsert',
    revision: device.registryRevision ?? db.registryRevision,
    payload: {
      ...payload,
      eventKind,
    },
  })
}

function publishDeviceRemove(device: DeviceRecord | undefined, revision: number): void {
  publishRealtimeMessage({
    topic: 'device.remove',
    revision,
    payload: {
      ...(device ? decorateDeviceRecord(device, revision) : { record: { id: 0, typeName: '', configRevision: revision }, config: {}, runtime: {} }),
      deviceId: device?.record?.id ?? device?.deviceId ?? 0,
      eventKind: 'device_deleted',
      registryRevision: revision,
    },
  })
}

export function connectMockRealtimeSocket(pinia: Pinia): MockRealtimeSocketHandle {
  const appStore = useAppStore(pinia)
  const wsStore = useWebSocketStore(pinia)
  const schedulePersistenceFlush = (): void => {
    void window
  }

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
      if (typeof window !== 'undefined' && window.__gekkoMockRealtimeRuntime?.schedulePersistenceFlush === schedulePersistenceFlush) {
        window.__gekkoMockRealtimeRuntime = undefined
      }
      markDisconnected()
    },
    publish(message: RealtimeMessage): void {
      publishRealtimeMessage(message)
    },
    upsertDevice(device: DeviceRecord): void {
      const db = loadMockDatabase()
      const index = db.devices.findIndex(entry => entry.deviceId === device.deviceId)
      if (index >= 0) {
        db.devices.splice(index, 1, device)
      } else {
        db.devices.push(device)
      }
      refreshMockDerivedDeviceState(db)
      db.registryRevision += 1
      saveMockDatabase(db)
      publishDeviceUpsert({
        ...device,
        registryRevision: db.registryRevision,
      }, 'device_updated')
      publishThermostatDependents(db, device.deviceId ?? 0)
    },
    removeDevice(deviceId: number): void {
      const db = loadMockDatabase()
      const removedDevice = db.devices.find(entry => entry.deviceId === deviceId)
      db.devices = db.devices.filter(entry => entry.deviceId !== deviceId)
      refreshMockDerivedDeviceState(db)
      db.registryRevision += 1
      saveMockDatabase(db)
      publishDeviceRemove(removedDevice, db.registryRevision)
      publishThermostatDependents(db, deviceId)
    },
    refreshSnapshot(): void {
      publishMockSnapshot()
    },
    schedulePersistenceFlush(): void {
      schedulePersistenceFlush()
    },
  }

  if (typeof window !== 'undefined') {
    window.__gekkoMockRealtime = handle
    window.__gekkoMockRealtimeRuntime = {
      schedulePersistenceFlush,
    }
  }

  publishMockSnapshot()
  markConnected()

  return handle
}
