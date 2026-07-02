import { defineStore } from 'pinia'

export type NotificationColor = 'success' | 'error' | 'info' | 'warning'

export interface NotificationEntry {
  id: number
  message: string
  color: NotificationColor
  timeout: number
}

let nextId = 1

export const useNotificationsStore = defineStore('v2-notifications', {
  state: () => ({
    queue: [] as NotificationEntry[],
  }),
  actions: {
    notify(message: string, color: NotificationColor = 'success', timeout = 4000): void {
      this.queue.push({ id: nextId++, message, color, timeout })
    },
    dismiss(id: number): void {
      this.queue = this.queue.filter(entry => entry.id !== id)
    },
  },
})
