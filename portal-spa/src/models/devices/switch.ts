import type { DeviceCommandRequest } from '../../api/contracts'

export type SwitchState = boolean

export const switchStateOptions: SwitchState[] = [false, true]

export function switchStateLabelKey(state: SwitchState): string {
  return state ? 'labels.output.on' : 'labels.output.off'
}

export function switchCommandPayload(state: SwitchState): DeviceCommandRequest {
  return {
    command: 'setOutput',
    state,
  }
}

export function nextDashboardPowerState(state: SwitchState | undefined): SwitchState {
  return !(state ?? false)
}
