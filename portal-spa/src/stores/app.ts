import { defineStore } from 'pinia'

import { applyLocale, detectInitialLocale, type AppLocale } from '@/i18n'
import {
  detectMockReset,
  detectTransportMode,
  setTransportMode as persistTransportMode,
  type TransportMode,
} from '@/api/transport'
import { safeReadStorage, safeWriteStorage } from '@/utils/storage'

const themeStorageKey = 'gekko.theme'

export type PortalMode = 'ap' | 'station'
export type WebSocketStatus = 'connected' | 'disconnected'
export type AppTheme = 'light' | 'dark'

function detectInitialTheme(): AppTheme {
  if (typeof window !== 'undefined') {
    const previewTheme = new URLSearchParams(window.location.search).get('theme')
    if (previewTheme === 'dark' || previewTheme === 'light') {
      return previewTheme
    }
  }

  const stored = safeReadStorage(themeStorageKey)
  if (stored === 'dark') {
    return 'dark'
  }
  if (stored === 'light') {
    return 'light'
  }
  if (typeof window !== 'undefined' && window.matchMedia?.('(prefers-color-scheme: dark)').matches) {
    return 'dark'
  }
  return 'light'
}

export const useAppStore = defineStore('app', {
  state: () => ({
    initialized: false,
    locale: detectInitialLocale(),
    theme: detectInitialTheme() as AppTheme,
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
    async setLocale(locale: AppLocale): Promise<void> {
      this.locale = locale
      await applyLocale(locale)
    },
    setTheme(theme: AppTheme): void {
      this.theme = theme
      safeWriteStorage(themeStorageKey, theme)
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
