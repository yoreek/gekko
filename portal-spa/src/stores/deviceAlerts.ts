import { defineStore } from 'pinia'

import { resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
import type { DeviceAlertSeverity } from '@/components/devices/registry/device-ui-types'
import { useDeviceRegistryStore } from './deviceRegistry'

export interface ActiveDeviceAlert {
  id: string
  deviceId: number
  deviceName: string
  severity: DeviceAlertSeverity
  messageKey: string
  messageParams?: Record<string, unknown>
}

export const useDeviceAlertsStore = defineStore('deviceAlerts', {
  getters: {
    activeAlerts(): ActiveDeviceAlert[] {
      const registry = useDeviceRegistryStore()
      const alerts: ActiveDeviceAlert[] = []
      for (const device of registry.devices) {
        const extractAlerts = resolveDeviceUi(device.record.typeName).extractAlerts
        if (extractAlerts === undefined) {
          continue
        }
        for (const alert of extractAlerts(device)) {
          alerts.push({
            id: `${device.record.id}:${alert.kind}`,
            deviceId: device.record.id,
            deviceName: device.config.name,
            severity: alert.severity,
            messageKey: alert.messageKey,
            messageParams: alert.messageParams,
          })
        }
      }
      return alerts.sort((a, b) => (a.severity === b.severity ? 0 : a.severity === 'critical' ? -1 : 1))
    },
    hasCritical(): boolean {
      return this.activeAlerts.some(alert => alert.severity === 'critical')
    },
  },
})
