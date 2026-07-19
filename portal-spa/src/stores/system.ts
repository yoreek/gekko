import { defineStore } from 'pinia'

import type { SystemStatusResponse } from '@/api'
import { fetchSystemStatus, fetchSystemVersion } from '@/api'

export const useSystemStore = defineStore('system', {
  state: () => ({
    status: 'idle',
    rebooting: false,
    firmwareVersion: '',
    firmwareBuildDate: '',
    controllerStatus: null as SystemStatusResponse | null,
  }),
  actions: {
    replaceFromResponse(payload: { status?: string; rebooting?: boolean }): void {
      this.status = payload.status ?? this.status
      this.rebooting = Boolean(payload.rebooting)
    },
    async loadFirmwareVersion(): Promise<void> {
      const response = await fetchSystemVersion()
      this.firmwareVersion = response.version
      this.firmwareBuildDate = response.buildDate
    },
    async loadControllerStatus(): Promise<void> {
      this.controllerStatus = await fetchSystemStatus()
    },
  },
})

