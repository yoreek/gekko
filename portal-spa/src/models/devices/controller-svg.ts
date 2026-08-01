import type { BoardPinCapability, BoardPinoutLayout } from '@/data/board-pin-capabilities'
import { buildControllerPinAnchors, type ControllerPinAnchor } from './controller-pin-anchors.ts'

export interface ControllerSvgModel {
  width: number
  height: number
  body: { x: number; y: number; width: number; height: number; rx: number }
  anchors: ControllerPinAnchor[]
}

export const CONTROLLER_SVG_METRICS = {
  width: 520,
  leftX: 75,
  rightX: 445,
  firstRowY: 36,
  rowHeight: 22,
}

export function buildControllerSvgModel(
  pins: readonly BoardPinCapability[],
  layout?: BoardPinoutLayout,
): ControllerSvgModel {
  const anchors = buildControllerPinAnchors(pins, layout, CONTROLLER_SVG_METRICS)
  const rowCount = Math.max(
    layout ? Math.max(layout.left.length, layout.right.length) : Math.ceil(pins.length / 2),
    1,
  )
  const height = CONTROLLER_SVG_METRICS.firstRowY + rowCount * CONTROLLER_SVG_METRICS.rowHeight + 28
  return {
    width: CONTROLLER_SVG_METRICS.width,
    height,
    body: { x: 130, y: 16, width: 260, height: height - 32, rx: 12 },
    anchors,
  }
}
