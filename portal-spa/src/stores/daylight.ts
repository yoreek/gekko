import { defineStore } from 'pinia'

import {
  normalizeDaylightCoordinates,
  type DaylightCoordinates,
} from '@/models/devices/analog-daylight'
import { safeReadStorage, safeWriteStorage } from '@/utils/storage'

export type DaylightLocationStatus =
  | 'idle'
  | 'requesting'
  | 'ready'
  | 'denied'
  | 'unavailable'

const storageKey = 'gekko.analog-daylight-location.v1'

function readCoordinates(): DaylightCoordinates | null {
  const stored = safeReadStorage(storageKey)
  if (stored === null) {
    return null
  }
  try {
    return normalizeDaylightCoordinates(JSON.parse(stored))
  } catch {
    return null
  }
}

export const useDaylightStore = defineStore('daylight', {
  state: () => ({
    coordinates: readCoordinates() as DaylightCoordinates | null,
    locationStatus: 'idle' as DaylightLocationStatus,
  }),
  actions: {
    async requestBrowserLocation(): Promise<void> {
      if (typeof navigator === 'undefined' || navigator.geolocation === undefined) {
        this.locationStatus = 'unavailable'
        return
      }
      this.locationStatus = 'requesting'
      await new Promise<void>(resolve => {
        navigator.geolocation.getCurrentPosition(
          position => {
            this.coordinates = {
              latitude: position.coords.latitude,
              longitude: position.coords.longitude,
            }
            safeWriteStorage(storageKey, JSON.stringify(this.coordinates))
            this.locationStatus = 'ready'
            resolve()
          },
          error => {
            this.locationStatus = error.code === 1
              ? 'denied'
              : 'unavailable'
            resolve()
          },
          {
            enableHighAccuracy: false,
            maximumAge: 86_400_000,
            timeout: 10_000,
          },
        )
      })
    },
  },
})
