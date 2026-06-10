import { defineStore } from 'pinia'

import { detectInitialLocale, type AppLocale } from '@/i18n'
import { detectMockReset, detectTransportMode, setTransportMode as persistTransportMode, type TransportMode } from '@/api/transport'
import { safeWriteStorage } from '@/utils/storage'

const storageKey = 'gekko.locale'

export type PortalMode = 'ap' | 'station'
export type WebSocketStatus = 'connected' | 'disconnected'

export const useAppStore = defineStore('app', {
  state: () => ({
    initialized: false,
    locale: detectInitialLocale(),
    transportMode: detectTransportMode() as TransportMode,
    mockResetRequested: detectMockReset(),
    mode: 'ap' as PortalMode,
    wifiStatus: 'idle',
    wsStatus: 'disconnected' as WebSocketStatus,
    deviceCount: 0,
    registryRevision: 0,
    otaEnabled: true,
    systemStatus: 'idle',
  }),
  actions: {
    initializeApp(): void {
      if (this.initialized) {
        return
      }
      this.initialized = true
    },
    setLocale(locale: AppLocale): void {
      this.locale = locale
      safeWriteStorage(storageKey, locale)
    },
    setTransportMode(mode: TransportMode): void {
      this.transportMode = mode
      persistTransportMode(mode)
    },
    setMockMode(enabled: boolean): void {
      this.setTransportMode(enabled ? 'mock' : 'real')
    },
    consumeMockReset(): boolean {
      const requested = this.mockResetRequested
      this.mockResetRequested = false
      return requested
    },
    setMode(mode: PortalMode): void {
      this.mode = mode
    },
    setWebSocketStatus(status: WebSocketStatus): void {
      this.wsStatus = status
    },
  },
})
