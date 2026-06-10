export interface RealtimeMessage<TPayload = unknown> {
  topic: string
  revision: number
  payload: TPayload
}

