import { defineStore } from 'pinia'

export type NotificationColor = 'success' | 'error' | 'info' | 'warning'

export interface NotificationEntry {
  text: string
  color: NotificationColor
  timeout: number
}

export const useNotificationsStore = defineStore('notifications', {
  state: () => ({
    queue: [] as NotificationEntry[],
  }),
  actions: {
    notify(text: string, color: NotificationColor = 'success', timeout = 4000): void {
      this.queue.push({ text, color, timeout })
    },
  },
})
