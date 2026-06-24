import type { DeviceCommandRequest, DeviceOutputState } from '../../api/contracts'

export type OutputState = DeviceOutputState

export const outputStateOptions: OutputState[] = ['off', 'on', 'disabled']

export function outputStateLabelKey(state: OutputState): string {
  return `labels.output.${state}`
}

export function switchCommandPayload(state: OutputState): DeviceCommandRequest {
  return {
    command: 'setOutput',
    state,
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
