import type { DeviceRecord } from '@/api/contracts'

export interface DeviceHardwarePin {
  gpio: number
  label: string
}

const PIN_LABELS: Record<string, string> = {
  sdaPin: 'SDA',
  sclPin: 'SCL',
  sckPin: 'SCK',
  mosiPin: 'MOSI',
  misoPin: 'MISO',
  chipSelectPin: 'CS',
  gpioPin: 'GPIO',
  pin: 'GPIO',
  dataPin: 'DATA',
  clockPin: 'CLOCK',
  latchPin: 'LATCH',
  resetPin: 'RESET',
  dcPin: 'DC',
  rstPin: 'RESET',
  clkPin: 'CLK',
  dioPin: 'DIO',
  rsPin: 'RS',
  ePin: 'E',
  d4Pin: 'D4',
  d5Pin: 'D5',
  d6Pin: 'D6',
  d7Pin: 'D7',
  backlightPin: 'BACKLIGHT',
  enablePin: 'ENABLE',
  sigPin: 'SIG',
}

function pinLabel(key: string): string | undefined {
  if (PIN_LABELS[key]) return PIN_LABELS[key]
  if (/^selectPins$/i.test(key)) return 'SELECT'
  if (/^(gpio|pin)\d+$/i.test(key)) return key.toUpperCase()
  return undefined
}

export function extractDeviceHardwarePins(device: DeviceRecord): DeviceHardwarePin[] {
  const config = device.config as unknown as Record<string, unknown>
  return Object.entries(config)
    .flatMap(([key, value]) => {
      const label = pinLabel(key)
      if (!label) return []
      const values = Array.isArray(value) ? value.map((entry, index) => ({ value: entry, label: `${label}${index}` })) : [{ value, label }]
      return values.flatMap(({ value: candidate, label: itemLabel }) => {
        if (typeof candidate !== 'number' || !Number.isInteger(candidate) || candidate < 0 || candidate === 255) return []
        return [{ gpio: candidate, label: itemLabel }]
      })
    })
    .sort((a, b) => a.gpio - b.gpio || a.label.localeCompare(b.label))
}
