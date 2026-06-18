export interface OneWireBusConfigDraft {
  enabled: boolean
  gpio_pin: number
  internal_pullup: boolean
}

export function createDefaultOneWireBusConfig(): OneWireBusConfigDraft {
  return {
    enabled: true,
    gpio_pin: 4,
    internal_pullup: false,
  }
}

export function normalizeOneWireBusConfig(value: unknown): OneWireBusConfigDraft {
  const defaults = createDefaultOneWireBusConfig()
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return defaults
  }
  const raw = value as Record<string, unknown>
  return {
    enabled: typeof raw.enabled === 'boolean' ? raw.enabled : defaults.enabled,
    gpio_pin: typeof raw.gpio_pin === 'number' && Number.isFinite(raw.gpio_pin) ? raw.gpio_pin : defaults.gpio_pin,
    internal_pullup: typeof raw.internal_pullup === 'boolean' ? raw.internal_pullup : defaults.internal_pullup,
  }
}
