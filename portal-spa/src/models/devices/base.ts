export interface DeviceCreateDraftBase {
  name: string
  type_id: number
  enabled: boolean
  config?: Record<string, unknown>
}
