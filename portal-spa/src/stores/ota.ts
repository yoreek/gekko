import { defineStore } from 'pinia'

import type { OtaStatusResponse } from '@/api'
import { fetchOtaStatus } from '@/api'

export const useOtaStore = defineStore('ota', {
  state: () => ({
    enabled: false,
    freeSketchSpace: 0,
    hasError: false,
    status: 'idle',
  }),
  actions: {
    replaceFromResponse(payload: OtaStatusResponse): void {
      this.enabled = payload.enabled
      this.freeSketchSpace = payload.freeSketchSpace
      this.hasError = payload.hasError
      this.status = payload.status ?? 'ok'
    },
    // Older firmware builds without web OTA answer 404 on the status route;
    // treat that as "OTA not compiled in" instead of surfacing an error.
    markUnavailable(): void {
      this.enabled = false
      this.freeSketchSpace = 0
      this.hasError = false
      this.status = 'unavailable'
    },
    async loadStatus(): Promise<void> {
      try {
        this.replaceFromResponse(await fetchOtaStatus())
      } catch {
        this.markUnavailable()
      }
    },
  },
})
