import { publishRealtimeMessage } from '@/realtime/bus'
import { loadMockDatabase } from './database'
import { decorateDeviceRecord, refreshMockDerivedDeviceState } from './handlers'

export function publishMockSnapshot(): void {
  const db = loadMockDatabase()
  refreshMockDerivedDeviceState(db)

  publishRealtimeMessage({
    topic: 'hello',
    revision: db.registryRevision,
    payload: {
      state: 'connected',
      clients: 1,
      registryRevision: db.registryRevision,
    },
  })

  publishRealtimeMessage({
    topic: 'wifi.status',
    revision: db.registryRevision,
    payload: {
      wifiStatus: db.wifi.status,
      stationIp: db.wifi.stationIp,
      setupApIp: db.wifi.setupApIp,
    },
  })

  publishRealtimeMessage({
    topic: 'ota.status',
    revision: db.registryRevision,
    payload: db.ota,
  })

  publishRealtimeMessage({
    topic: 'system.status',
    revision: db.registryRevision,
    payload: db.system,
  })

  for (const device of db.devices) {
    publishRealtimeMessage({
      topic: 'device.upsert',
      revision: db.registryRevision,
      payload: {
        ...decorateDeviceRecord(device, db.registryRevision),
        eventKind: 'snapshot',
      },
    })
  }
}
