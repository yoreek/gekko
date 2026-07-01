import type { Component } from 'vue'

import type { PortalIconName } from '@/icons'
import type { DeviceTypeId } from '@/models/device-type-ids'

export interface DeviceUi {
  readonly typeId: DeviceTypeId
  readonly typeName: string
  readonly labelKey: string
  readonly icon: PortalIconName
  readonly editorComponent: Component
  readonly widgetComponent: Component
  readonly designerComponent?: Component
  readonly designDisplayLabelKey?: string
  readonly layoutResizeWarningLabelKey?: string
}
