import type { Pinia } from 'pinia'

import { publishRealtimeMessage } from './bus'
import type { RealtimeMessage } from './messages'
import { publishMockSnapshot } from '@/mock/snapshot'
import { canonicalizeDeviceRecord, loadMockDatabase, saveMockDatabase, type MockDeviceRecord } from '@/mock/database'
import { decorateDeviceRecord, publishThermostatDependents, refreshMockDerivedDeviceState } from '@/mock/handlers'
import { useAppStore } from '@/stores/app'
import { useWebSocketStore } from '@/stores/websocket'

type DeviceRecord = MockDeviceRecord

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
  const payload = decorateDeviceRecord(device, db.registryRevision)
  publishRealtimeMessage({
    topic: 'device.upsert',
    revision: db.registryRevision,
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
    payload: device ? decorateDeviceRecord(device, revision) : { record: { id: 0, typeName: '', configRevision: revision }, config: {}, runtime: {} },
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
      const canonicalDevice = canonicalizeDeviceRecord(device)
      const index = db.devices.findIndex(entry => entry.record.id === canonicalDevice.record.id)
      if (index >= 0) {
        db.devices.splice(index, 1, canonicalDevice)
      } else {
        db.devices.push(canonicalDevice)
      }
      refreshMockDerivedDeviceState(db)
      db.registryRevision += 1
      saveMockDatabase(db)
      publishDeviceUpsert({
        ...canonicalDevice,
      }, 'device_updated')
      publishThermostatDependents(db, canonicalDevice.record.id)
    },
    removeDevice(deviceId: number): void {
      const db = loadMockDatabase()
      const removedDevice = db.devices.find(entry => entry.record.id === deviceId)
      db.devices = db.devices.filter(entry => entry.record.id !== deviceId)
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
