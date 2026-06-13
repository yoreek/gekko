import { defineStore } from 'pinia'

import type { RealtimeMessage } from '@/realtime/messages'

export const useWebSocketStore = defineStore('websocket', {
  state: () => ({
    connected: false,
    revision: 0,
    lastTopic: '',
  }),
  actions: {
    markConnected(): void {
      this.connected = true
    },
    markDisconnected(): void {
      this.connected = false
    },
    applyMessage(message: RealtimeMessage): void {
      this.revision = message.revision
      this.lastTopic = message.topic
      if (message.topic === 'hello') {
        this.connected = true
      }
    },
  },
})

