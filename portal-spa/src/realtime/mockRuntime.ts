export interface MockRealtimeRuntimeHandle {
  schedulePersistenceFlush?: () => void
}

declare global {
  interface Window {
    __gekkoMockRealtimeRuntime?: MockRealtimeRuntimeHandle
  }
}

export function scheduleMockPersistenceFlush(): void {
  if (typeof window === 'undefined') {
    return
  }

  window.__gekkoMockRealtimeRuntime?.schedulePersistenceFlush?.()
}
