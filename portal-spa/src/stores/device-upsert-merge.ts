import type { DeviceRecord } from '@/api/contracts'

// Realtime device.upsert pushes (e.g. a routine sensor reading update) are built from just the
// device's own IDeviceApiAdapter::writeDeviceJson and never include `ha` -- only a REST fetch or
// a setHaSettings command response does. Without this, every periodic push would wipe out the
// previously-fetched `ha` field and flicker the HA settings card off, even though nothing about
// the device's HA settings actually changed.
export function mergeUpsertedDevice(previous: DeviceRecord | undefined, incoming: DeviceRecord): DeviceRecord {
  if (previous !== undefined && incoming.ha === undefined && previous.ha !== undefined) {
    return { ...incoming, ha: previous.ha }
  }
  return incoming
}
