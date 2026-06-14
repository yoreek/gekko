import type { DeviceCommandRequest, DeviceOutputState } from '../../api/contracts'

export type OutputState = DeviceOutputState

export interface SwitchConfigDraft {
  restore_previous_state: boolean
  startup_state: OutputState
  safe_state: OutputState
  inverted: boolean
}

export const outputStateOptions: OutputState[] = ['off', 'on', 'disabled']

export function isOutputState(value: unknown): value is OutputState {
  return value === 'off' || value === 'on' || value === 'disabled'
}

export function outputStateLabelKey(state: OutputState): string {
  return `labels.output.${state}`
}

export function switchCommandPayload(state: OutputState): DeviceCommandRequest {
  return {
    command: 'custom',
    payload: `state=${state}`,
  }
}

export function nextDashboardPowerState(state: OutputState | undefined): OutputState | null {
  if (state === 'on') {
    return 'off'
  }
  if (state === 'off') {
    return 'on'
  }
  return null
}
