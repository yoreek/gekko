import { defineStore } from 'pinia'

import type { OtaStatusResponse } from '@/api'

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
      this.freeSketchSpace = payload.free_sketch_space
      this.hasError = payload.has_error
      this.status = payload.status ?? 'ok'
    },
  },
})

