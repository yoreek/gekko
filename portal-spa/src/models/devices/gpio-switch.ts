import { isOutputState, type OutputState, type SwitchConfigDraft } from './switch'

export interface GpioSwitchConfigDraft extends SwitchConfigDraft {
  gpio_pin: number
}

export function createDefaultGpioSwitchConfig(): GpioSwitchConfigDraft {
  return {
    restore_previous_state: false,
    startup_state: 'off',
    safe_state: 'disabled',
    inverted: false,
    gpio_pin: 4,
  }
}

function readOutputState(value: unknown, fallback: OutputState): OutputState {
  return isOutputState(value) ? value : fallback
}

export function normalizeGpioSwitchConfig(value: unknown): GpioSwitchConfigDraft {
  const defaults = createDefaultGpioSwitchConfig()
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return defaults
  }
  const raw = value as Record<string, unknown>
  return {
    restore_previous_state:
      typeof raw.restore_previous_state === 'boolean' ? raw.restore_previous_state : defaults.restore_previous_state,
    startup_state: readOutputState(raw.startup_state, defaults.startup_state),
    safe_state: readOutputState(raw.safe_state, defaults.safe_state),
    inverted: typeof raw.inverted === 'boolean' ? raw.inverted : defaults.inverted,
    gpio_pin: typeof raw.gpio_pin === 'number' && Number.isFinite(raw.gpio_pin) ? raw.gpio_pin : defaults.gpio_pin,
  }
}
