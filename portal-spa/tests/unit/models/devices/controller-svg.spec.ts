import assert from 'node:assert/strict'
import test from 'node:test'

import type { BoardPinCapability, BoardPinoutLayout } from '../../../../src/data/board-pin-capabilities.ts'
import { buildControllerSvgModel, CONTROLLER_SVG_METRICS } from '../../../../src/models/devices/controller-svg.ts'

const pins: BoardPinCapability[] = [
  { gpio: 21, roles: ['input', 'output'] },
  { gpio: 22, roles: ['input', 'output'] },
]

test('buildControllerSvgModel creates a board body and one anchor per layout pin', () => {
  const layout: BoardPinoutLayout = { left: ['EN', 21], right: [22, 'GND'] }
  const model = buildControllerSvgModel(pins, layout)
  assert.equal(model.width, CONTROLLER_SVG_METRICS.width)
  assert.equal(model.body.x + model.body.width < model.width, true)
  assert.deepEqual(model.anchors.map(anchor => [anchor.label, anchor.x, anchor.y]), [
    ['EN', 75, 36],
    ['GPIO21', 75, 58],
    ['GPIO22', 445, 36],
    ['GND', 445, 58],
  ])
})

test('controller SVG GPIO anchor coordinates are unique and stable', () => {
  const model = buildControllerSvgModel(pins, { left: [21], right: [22] })
  const gpioAnchors = model.anchors.filter(anchor => anchor.gpio !== undefined)
  assert.equal(new Set(gpioAnchors.map(anchor => `${anchor.x}:${anchor.y}`)).size, gpioAnchors.length)
  assert.deepEqual(gpioAnchors.map(anchor => anchor.id), ['gpio-21', 'gpio-22'])
})
