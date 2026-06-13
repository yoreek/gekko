import type { Pinia } from 'pinia'

import { resolveApiBase } from '@/api/http'
import { useAppStore } from '@/stores/app'
import { useWebSocketStore } from '@/stores/websocket'

import { publishRealtimeMessage } from './bus'
import type { RealtimeMessage } from './messages'

type RealtimeSocketHandle = {
  dispose: () => void
}

function buildRealtimeUrl(): string {
  const base = resolveApiBase()
  const source = base || window.location.origin
  const url = new URL('/ws', source)
  url.protocol = url.protocol === 'https:' ? 'wss:' : 'ws:'
  return url.toString()
}

function isRealtimeMessage(value: unknown): value is RealtimeMessage {
  if (typeof value !== 'object' || value === null) {
    return false
  }

  const message = value as Partial<RealtimeMessage>
  return typeof message.topic === 'string' && typeof message.revision === 'number' && 'payload' in message
}

export function connectRealtimeSocket(pinia: Pinia): RealtimeSocketHandle {
  const appStore = useAppStore(pinia)
  const wsStore = useWebSocketStore(pinia)

  let socket: WebSocket | null = null
  let reconnectTimer: number | null = null
  let disposed = false
  let reconnectDelayMs = 1000

  const clearReconnectTimer = (): void => {
    if (reconnectTimer === null) {
      return
    }
    window.clearTimeout(reconnectTimer)
    reconnectTimer = null
  }

  const setDisconnected = (): void => {
    wsStore.markDisconnected()
    appStore.setWebSocketStatus('disconnected')
  }

  const scheduleReconnect = (): void => {
    if (disposed || reconnectTimer !== null) {
      return
    }

    reconnectTimer = window.setTimeout(() => {
      reconnectTimer = null
      reconnectDelayMs = Math.min(reconnectDelayMs * 2, 10000)
      openSocket()
    }, reconnectDelayMs)
  }

  const openSocket = (): void => {
    if (disposed) {
      return
    }

    clearReconnectTimer()

    try {
      socket = new WebSocket(buildRealtimeUrl())
    } catch {
      setDisconnected()
      scheduleReconnect()
      return
    }

    socket.addEventListener('open', () => {
      reconnectDelayMs = 1000
      wsStore.markConnected()
      appStore.setWebSocketStatus('connected')
    })

    socket.addEventListener('message', event => {
      if (typeof event.data !== 'string') {
        return
      }

      let parsed: unknown
      try {
        parsed = JSON.parse(event.data)
      } catch {
        return
      }

      if (!isRealtimeMessage(parsed)) {
        return
      }

      publishRealtimeMessage(parsed)
    })

    socket.addEventListener('close', () => {
      socket = null
      setDisconnected()
      scheduleReconnect()
    })

    socket.addEventListener('error', () => {
      setDisconnected()
      scheduleReconnect()
    })
  }

  openSocket()

  return {
    dispose(): void {
      disposed = true
      clearReconnectTimer()
      if (socket !== null) {
        socket.close()
        socket = null
      }
      setDisconnected()
    },
  }
}
