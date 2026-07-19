import type { RealtimeMessage } from './messages'

type Listener = (message: RealtimeMessage) => void

const listeners = new Set<Listener>()

export function publishRealtimeMessage<TPayload>(message: RealtimeMessage<TPayload>): void {
  for (const listener of listeners) {
    listener(message)
  }
}

export function subscribeRealtimeMessage(listener: Listener): () => void {
  listeners.add(listener)
  return () => listeners.delete(listener)
}

