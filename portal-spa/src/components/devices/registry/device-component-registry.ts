import type { Component } from 'vue'

import { resolveDeviceComponent } from '@/models/device-types'

export function resolveDashboardDeviceComponent(typeId: number): Component {
  return resolveDeviceComponent(typeId)
}
