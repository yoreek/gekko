import { safeReadStorage, safeWriteStorage } from '@/utils/storage'

export type TransportMode = 'real' | 'mock'

const transportKey = 'gekko.transportMode'

function normalizeMode(value: string | null | undefined): TransportMode {
  return value === 'mock' ? 'mock' : 'real'
}

function readQueryParam(name: string): string | null {
  if (typeof window === 'undefined') {
    return null
  }
  return new URL(window.location.href).searchParams.get(name)
}

export function detectTransportMode(): TransportMode {
  const queryMode = readQueryParam('mockMode')
  if (queryMode !== null) {
    return normalizeMode(queryMode === '1' || queryMode === 'true' || queryMode === 'mock' ? 'mock' : 'real')
  }

  if (typeof window !== 'undefined') {
    const storedMode = safeReadStorage(transportKey)
    if (storedMode !== null) {
      return normalizeMode(storedMode)
    }
  }

  return normalizeMode((import.meta.env.VITE_MOCK_MODE as string | undefined) ?? 'real')
}

export function detectMockReset(): boolean {
  const queryReset = readQueryParam('mockReset')
  if (queryReset === null) {
    return false
  }
  return queryReset === '1' || queryReset === 'true' || queryReset === 'mock'
}

export function setTransportMode(mode: TransportMode): void {
  safeWriteStorage(transportKey, mode)
}
