import { defineStore } from 'pinia'

import { fetchDevices, type DeviceRecord, type DeviceRegistryResponse } from '@/api'
import { normalizeDeviceCollection, normalizeDeviceRecord } from '@/models/device'

export const useDeviceRegistryStore = defineStore('deviceRegistry', {
  state: () => ({
    devices: [] as DeviceRecord[],
    registryRevision: 0,
    initialized: false,
    initializePromise: null as Promise<void> | null,
  }),
  actions: {
    replaceFromResponse(payload: DeviceRegistryResponse): void {
      const collection = normalizeDeviceCollection(payload)
      this.devices = [...collection.devices]
      this.registryRevision = collection.registryRevision
    },
    async initialize(): Promise<void> {
      if (this.initialized) {
        return
      }
      if (this.initializePromise !== null) {
        await this.initializePromise
        return
      }

      this.initializePromise = (async () => {
        try {
          const response = await fetchDevices()
          this.replaceFromResponse(response)
          this.initialized = true
        } finally {
          this.initializePromise = null
        }
      })()
      await this.initializePromise
    },
    async reload(): Promise<void> {
      if (this.initializePromise !== null) {
        await this.initializePromise
      }

      const response = await fetchDevices()
      this.replaceFromResponse(response)
      this.initialized = true
    },
    upsertDevice(device: DeviceRecord, revision: number): void {
      const normalized = normalizeDeviceRecord(device, revision)
      const index = this.devices.findIndex(entry => entry.record.id === normalized.record.id)
      if (index >= 0) {
        this.devices.splice(index, 1, normalized)
      } else {
        this.devices.push(normalized)
      }
      this.registryRevision = revision
    },
    removeDevice(deviceId: number, revision: number): void {
      this.devices = this.devices.filter(device => device.record.id !== deviceId)
      this.registryRevision = revision
    },
    setRevision(revision: number): void {
      this.registryRevision = revision
    },
  },
})
