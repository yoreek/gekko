import { publishRealtimeMessage } from '@/realtime/bus'
import { loadMockDatabase } from './database'
import { refreshMockDerivedDeviceState } from './handlers'

export function publishMockSnapshot(): void {
  const db = loadMockDatabase()
  refreshMockDerivedDeviceState(db)

  publishRealtimeMessage({
    topic: 'hello',
    revision: db.registryRevision,
    payload: {
      state: 'connected',
      clients: 1,
      registry_revision: db.registryRevision,
    },
  })

  publishRealtimeMessage({
    topic: 'wifi.status',
    revision: db.registryRevision,
    payload: {
      wifi_status: db.wifi.status,
      station_ip: db.wifi.stationIp,
      setup_ap_ip: db.wifi.setupApIp,
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
        ...device,
        registry_revision: db.registryRevision,
        pending_persistence: db.pendingPersistence,
      },
    })
  }
}
