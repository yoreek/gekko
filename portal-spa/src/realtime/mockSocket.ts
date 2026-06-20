import type { DeviceRecord } from '@/api'
import type { Pinia } from 'pinia'

import { publishRealtimeMessage } from './bus'
import type { RealtimeMessage } from './messages'
import { publishMockSnapshot } from '@/mock/snapshot'
import { loadMockDatabase, saveMockDatabase } from '@/mock/database'
import { publishThermostatDependents, refreshMockDerivedDeviceState } from '@/mock/handlers'
import { useAppStore } from '@/stores/app'
import { useWebSocketStore } from '@/stores/websocket'

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

function publishDeviceUpsert(device: DeviceRecord): void {
  const db = loadMockDatabase()
  publishRealtimeMessage({
    topic: 'device.upsert',
    revision: device.registry_revision ?? db.registryRevision,
    payload: {
      ...device,
      registry_revision: device.registry_revision ?? db.registryRevision,
      pending_persistence: device.pending_persistence ?? db.pendingPersistence,
    },
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
  let persistenceFlushTimer: number | null = null
  const persistenceFlushDelayMs = 250

  const clearPersistenceFlushTimer = (): void => {
    if (persistenceFlushTimer === null) {
      return
    }
    window.clearTimeout(persistenceFlushTimer)
    persistenceFlushTimer = null
  }

  const flushPendingPersistence = (): void => {
    persistenceFlushTimer = null
    const db = loadMockDatabase()
    if (!db.pendingPersistence) {
      return
    }

    db.pendingPersistence = false
    saveMockDatabase(db)

    for (const device of db.devices) {
      publishRealtimeMessage({
        topic: 'device.upsert',
        revision: db.registryRevision,
        payload: {
          ...device,
          pending_persistence: false,
          registry_revision: db.registryRevision,
        },
      })
    }
  }

  const schedulePersistenceFlush = (): void => {
    if (typeof window === 'undefined') {
      return
    }
    clearPersistenceFlushTimer()
    persistenceFlushTimer = window.setTimeout(flushPendingPersistence, persistenceFlushDelayMs)
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
      const index = db.devices.findIndex(entry => entry.device_id === device.device_id)
      if (index >= 0) {
        db.devices.splice(index, 1, device)
      } else {
        db.devices.push(device)
      }
      refreshMockDerivedDeviceState(db)
      db.registryRevision += 1
      db.pendingPersistence = true
      saveMockDatabase(db)
      publishDeviceUpsert({
        ...device,
        registry_revision: db.registryRevision,
        pending_persistence: db.pendingPersistence,
      })
      publishThermostatDependents(db, device.device_id)
    },
    removeDevice(deviceId: number): void {
      const db = loadMockDatabase()
      db.devices = db.devices.filter(entry => entry.device_id !== deviceId)
      refreshMockDerivedDeviceState(db)
      db.registryRevision += 1
      db.pendingPersistence = true
      saveMockDatabase(db)
      publishDeviceRemove(deviceId, db.registryRevision, db.pendingPersistence)
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
