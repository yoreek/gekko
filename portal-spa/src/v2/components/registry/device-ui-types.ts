import type { Component } from 'vue'

import type { PortalIconName } from '@/icons'
import type { DeviceTypeId } from '@/models/device-type-ids'

/**
 * Page-based device UI contract (v2). All components consume the same
 * DeviceEditDraft shape as v1 (@/components/device/device-form) so the
 * existing model layer (BaseDevice subclasses, useDeviceDetail) is reused
 * unchanged. fieldsComponent renders ONLY the type-specific fields; the
 * base fields (name/enabled/deps) are rendered once by DeviceBaseFields.
 */
export interface DeviceUiV2 {
  readonly typeId: DeviceTypeId
  readonly typeName: string
  readonly labelKey: string
  readonly icon: PortalIconName
  /** Props: { modelValue: DeviceEditDraft; mode: 'view'|'edit'|'create'; busy?: boolean }, emits: update:modelValue */
  readonly fieldsComponent: Component
  /** Compact card for dashboard grid. Props: { device: DeviceRecord } */
  readonly widgetComponent: Component
  /**
   * Full page designer (e.g. display layout editor). Props: { device: DeviceRecord | null },
   * emits: save(payload), cancel. MUST render as plain page content — no v-dialog/overlay.
   */
  readonly designerComponent?: Component
}
