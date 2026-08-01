import { defineStore } from 'pinia'

import { fetchPinOccupancy, type PinOccupancyResponse } from '@/api'

export const usePinOccupancyStore = defineStore('pinOccupancy', {
  state: () => ({
    // gpio -> owning deviceId. Absent entries are free.
    owners: {} as Record<number, number>,
  }),
  actions: {
    replaceFromResponse(payload: PinOccupancyResponse): void {
      const owners: Record<number, number> = {}
      for (const entry of payload.pins) {
        owners[entry.gpio] = entry.deviceId
      }
      this.owners = owners
    },
    // Firmware-authoritative occupancy changes whenever any device anywhere is created,
    // reconfigured or removed -- unlike board settings (rarely changes), this must be refetched
    // every time a device form opens rather than cached for the session. Cheap: at most
    // kGpioPinTableSize entries.
    async refresh(): Promise<void> {
      this.replaceFromResponse(await fetchPinOccupancy())
    },
  },
})
