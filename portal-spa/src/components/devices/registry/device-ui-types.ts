import type { Component } from 'vue'

import type { PortalIconName } from '@/icons'
import type { DeviceTypeId } from '@/models/device-type-ids'

/**
 * Page-based device UI contract (v2). All components consume the same
 * DeviceEditDraft shape as v1 (@/models/devices/device-draft) so the
 * existing model layer (BaseDevice subclasses, useDeviceDetail) is reused
 * unchanged. fieldsComponent renders ONLY the type-specific fields; the
 * base fields (name/enabled/deps) are rendered once by DeviceBaseFields.
 */
export interface DeviceUi {
  readonly typeId: DeviceTypeId
  readonly typeName: string
  readonly labelKey: string
  readonly icon: PortalIconName
  /**
   * Props: { modelValue: DeviceEditDraft; device?: DeviceRecord; mode: 'view'|'edit'|'create'; busy?: boolean }
   * Emits: update:modelValue, and optionally command(payload: DeviceCommandRequest) for types that expose
   * quick actions (e.g. gpio_switch output control, i2c/spi/onewire bus scan/diagnostics reset).
   */
  readonly fieldsComponent: Component
  /**
   * Renders the device's own status/controls at two densities from ONE component:
   * Props: { device: DeviceRecord; editable?: boolean; dense?: boolean } (dense defaults true).
   * dense=true  -> compact card body, used inside DeviceWidgetBase on the dashboard grid.
   * dense=false -> expanded body, used inside DeviceMoreInfoDialog's more-info modal
   *                (no card chrome — the dialog supplies its own DeviceIdentityHeader).
   * Emits: open() (dense mode only, bubbles to DeviceWidgetBase's click-to-open), and
   * optionally command(payload: DeviceCommandRequest) for types with quick/live actions
   * (e.g. gpio_switch power toggle, thermostat mode switch via updateConfig).
   */
  readonly widgetComponent: Component
  /**
   * Full page designer (e.g. display layout editor). Props: { device: DeviceRecord | null },
   * emits: save(payload), cancel. MUST render as plain page content — no v-dialog/overlay.
   */
  readonly designerComponent?: Component
}
