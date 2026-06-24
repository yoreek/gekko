import type { OutputState } from '@/models/devices/switch'

export interface SwitchConfigDraft {
  restorePreviousState: boolean
  startupState: OutputState
  safeState: OutputState
  inverted: boolean
}
