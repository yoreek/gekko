import type { BoardPinCapability, BoardPinoutLayout } from '@/data/board-pin-capabilities'

export interface ControllerPinAnchor {
  id: string
  gpio?: number
  label: string
  x: number
  y: number
  side: 'left' | 'right'
  kind: 'gpio' | 'power' | 'ground' | 'control' | 'other'
  row: number
}

export interface ControllerPinAnchorMetrics {
  width?: number
  leftX?: number
  rightX?: number
  firstRowY?: number
  rowHeight?: number
}

const DEFAULT_METRICS = {
  width: 440,
  leftX: 93,
  rightX: 347,
  firstRowY: 18,
  rowHeight: 22,
}

function pinKind(label: string): ControllerPinAnchor['kind'] {
  const normalized = label.toUpperCase()
  if (normalized.includes('GND')) return 'ground'
  if (normalized === '3V3' || normalized === '5V') return 'power'
  if (normalized === 'EN' || normalized === 'RST') return 'control'
  return 'other'
}

export function buildControllerPinAnchors(
  pins: readonly BoardPinCapability[],
  layout?: BoardPinoutLayout,
  metrics: ControllerPinAnchorMetrics = {},
): ControllerPinAnchor[] {
  const resolved = { ...DEFAULT_METRICS, ...metrics }
  const middle = Math.ceil(pins.length / 2)
  const left = layout?.left ?? pins.slice(0, middle).map(pin => pin.gpio)
  const right = layout?.right ?? pins.slice(middle).map(pin => pin.gpio)
  const pinByGpio = new Map(pins.map(pin => [pin.gpio, pin]))

  return [
    ...buildSideAnchors(left, 'left', resolved, pinByGpio),
    ...buildSideAnchors(right, 'right', resolved, pinByGpio),
  ]
}

function buildSideAnchors(
  entries: readonly (number | string)[],
  side: 'left' | 'right',
  metrics: typeof DEFAULT_METRICS,
  pinByGpio: ReadonlyMap<number, BoardPinCapability>,
): ControllerPinAnchor[] {
  return entries.map((entry, row) => {
    const gpio = typeof entry === 'number' ? entry : undefined
    const label = gpio === undefined ? String(entry) : `GPIO${gpio}`
    return {
      id: gpio === undefined ? `${side}-${label}-${row}` : `gpio-${gpio}`,
      gpio,
      label,
      x: side === 'left' ? metrics.leftX : metrics.rightX,
      y: metrics.firstRowY + row * metrics.rowHeight,
      side,
      kind: gpio === undefined ? pinKind(label) : (pinByGpio.has(gpio) ? 'gpio' : 'other'),
      row,
    }
  })
}
