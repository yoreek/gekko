import assert from 'node:assert/strict'
import test from 'node:test'

import type { BoardPinCapability, BoardPinoutLayout } from '../../../../src/data/board-pin-capabilities.ts'
import { buildControllerPinAnchors } from '../../../../src/models/devices/controller-pin-anchors.ts'

const pins: BoardPinCapability[] = [
  { gpio: 21, roles: ['input', 'output'] },
  { gpio: 22, roles: ['input', 'output'] },
  { gpio: 5, roles: ['input', 'output'] },
]

test('buildControllerPinAnchors preserves physical row positions including non-GPIO entries', () => {
  const layout: BoardPinoutLayout = {
    left: ['EN', 'GND', 21],
    right: [22, '3V3', 5],
  }
  const anchors = buildControllerPinAnchors(pins, layout)
  assert.deepEqual(anchors.filter(anchor => anchor.gpio).map(anchor => ({ id: anchor.id, x: anchor.x, y: anchor.y })), [
    { id: 'gpio-21', x: 93, y: 62 },
    { id: 'gpio-22', x: 347, y: 18 },
    { id: 'gpio-5', x: 347, y: 62 },
  ])
  assert.equal(anchors.find(anchor => anchor.label === 'GND')?.kind, 'ground')
  assert.equal(anchors.find(anchor => anchor.label === '3V3')?.kind, 'power')
})

test('buildControllerPinAnchors supports custom SVG metrics', () => {
  const [anchor] = buildControllerPinAnchors(pins, { left: [21], right: [] }, {
    leftX: 12,
    rightX: 88,
    firstRowY: 10,
    rowHeight: 30,
  })
  assert.deepEqual(anchor, {
    id: 'gpio-21',
    gpio: 21,
    label: 'GPIO21',
    x: 12,
    y: 10,
    side: 'left',
    kind: 'gpio',
    row: 0,
  })
})
