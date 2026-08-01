import { defineStore } from 'pinia'

import type { BoardSettingsResponse } from '@/api'

export const useBoardStore = defineStore('board', {
  state: () => ({
    chip: 'esp32',
    selectedBoardId: '',
    supportedBoardIds: [] as string[],
  }),
  actions: {
    replaceFromSettings(payload: BoardSettingsResponse): void {
      this.chip = payload.chip
      this.selectedBoardId = payload.selectedBoardId
      this.supportedBoardIds = payload.supportedBoardIds
    },
  },
})
