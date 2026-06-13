import { defineStore } from 'pinia'

import type { DeviceRecord, DeviceRegistryResponse } from '@/api'
import { normalizeDeviceCollection, normalizeDeviceRecord, type DashboardDevice } from '@/models/device'

export const useDeviceRegistryStore = defineStore('deviceRegistry', {
  state: () => ({
    devices: [] as DashboardDevice[],
    registryRevision: 0,
    pendingPersistence: false,
  }),
  actions: {
    replaceFromResponse(payload: DeviceRegistryResponse): void {
      const collection = normalizeDeviceCollection(payload)
      this.devices = [...collection.devices]
      this.registryRevision = collection.registryRevision
      this.pendingPersistence = collection.pendingPersistence
    },
    upsertDevice(device: DeviceRecord, revision: number): void {
      const normalized = normalizeDeviceRecord(device, revision, device.pending_persistence ?? this.pendingPersistence)
      const index = this.devices.findIndex(entry => entry.deviceId === normalized.deviceId)
      if (index >= 0) {
        this.devices.splice(index, 1, normalized)
      } else {
        this.devices.push(normalized)
      }
      this.registryRevision = revision
      if (typeof device.pending_persistence === 'boolean') {
        this.pendingPersistence = device.pending_persistence
      }
    },
    removeDevice(deviceId: number, revision: number): void {
      this.devices = this.devices.filter(device => device.deviceId !== deviceId)
      this.registryRevision = revision
    },
    setRevision(revision: number): void {
      this.registryRevision = revision
    },
    setPendingPersistence(pendingPersistence: boolean): void {
      this.pendingPersistence = pendingPersistence
    },
  },
})
