import type { DeviceCommandRequest, DeviceOutputState, GpioSwitchOutputSnapshot } from '../../api/contracts'

export type OutputState = DeviceOutputState

export interface SwitchConfigDraft {
  restorePreviousState: boolean
  startupState: OutputState
  safeState: OutputState
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

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

export function normalizeSwitchOutput(value: unknown): GpioSwitchOutputSnapshot {
  if (!isRecord(value)) {
    return {}
  }
  return {
    state: isOutputState(value.state) ? value.state : undefined,
    physicalLevel: typeof value.physicalLevel === 'boolean' ? value.physicalLevel : undefined,
  }
}
