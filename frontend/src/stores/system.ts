import { defineStore } from 'pinia'

export const useSystemStore = defineStore('system', {
  state: () => ({
    status: 'idle',
    rebooting: false,
  }),
  actions: {
    replaceFromResponse(payload: { status?: string; rebooting?: boolean }): void {
      this.status = payload.status ?? this.status
      this.rebooting = Boolean(payload.rebooting)
    },
  },
})

