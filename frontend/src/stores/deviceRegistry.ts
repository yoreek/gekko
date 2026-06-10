import { defineStore } from 'pinia'

import type { DeviceRecord } from '@/api'

export const useDeviceRegistryStore = defineStore('deviceRegistry', {
  state: () => ({
    devices: [] as DeviceRecord[],
    registryRevision: 0,
    pendingPersistence: false,
  }),
  actions: {
    replaceFromResponse(payload: { devices: DeviceRecord[]; registry_revision: number; pending_persistence: boolean }): void {
      this.devices = [...payload.devices]
      this.registryRevision = payload.registry_revision
      this.pendingPersistence = payload.pending_persistence
    },
    upsertDevice(device: DeviceRecord, revision: number): void {
      const index = this.devices.findIndex(entry => entry.device_id === device.device_id)
      if (index >= 0) {
        this.devices.splice(index, 1, device)
      } else {
        this.devices.push(device)
      }
      this.registryRevision = revision
    },
    removeDevice(deviceId: number, revision: number): void {
      this.devices = this.devices.filter(device => device.device_id !== deviceId)
      this.registryRevision = revision
    },
    setRevision(revision: number): void {
      this.registryRevision = revision
    },
  },
})

