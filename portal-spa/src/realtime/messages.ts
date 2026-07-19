export interface RealtimeMessage<TPayload = unknown> {
  topic: string
  revision: number
  payload: TPayload
}

// The device.remove topic intentionally does not carry the full DeviceRecord (record/config/runtime) -
// only identity and removal metadata, matching PortalWebSocketMessages::buildDeviceRemove() on the
// firmware side (docs/device-model-structures.md, "Realtime device topics").
export interface DeviceRemoveEventPayload {
  eventKind?: string
  deviceId: number
  typeId: number
  registryRevision: number
  name?: string
  typeName?: string
  detail?: string
}
