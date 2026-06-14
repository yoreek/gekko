export interface DummyDeviceConfigDraft {
  restore_previous_state: boolean
  default_output: boolean
  current_output: boolean
  inverted: boolean
}

export function createDefaultDummyDeviceConfig(): DummyDeviceConfigDraft {
  return {
    restore_previous_state: false,
    default_output: false,
    current_output: false,
    inverted: false,
  }
}
