import { Pcf857xExpanderDeviceBase } from './pcf857x-expander.ts'
import type { Pcf857xExpanderConfigDraft, Pcf857xExpanderCreateDraft } from './pcf857x-expander.ts'

export type Pcf8574ExpanderConfigDraft = Pcf857xExpanderConfigDraft
export type Pcf8574ExpanderCreateDraft = Pcf857xExpanderCreateDraft

export class Pcf8574ExpanderDevice extends Pcf857xExpanderDeviceBase {
  static readonly TYPE_ID = 12 as const
  static readonly TYPE_NAME = 'pcf8574_expander' as const
  static readonly CHANNEL_COUNT = 8

  readonly typeName = Pcf8574ExpanderDevice.TYPE_NAME
  readonly typeId = Pcf8574ExpanderDevice.TYPE_ID
}
