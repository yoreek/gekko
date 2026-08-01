import assert from 'node:assert/strict'
import test from 'node:test'

import { routePhysicalEdge } from '../../../../src/models/devices/physical-edge-routing.ts'

const body = { x: 130, y: 16, width: 260, height: 450 }
const bounds = { x: 0, y: 0, width: 520, height: 482 }

test('routePhysicalEdge uses only horizontal and vertical segments', () => {
  const route = routePhysicalEdge({
    source: { x: 20, y: 120 },
    target: { x: 620, y: 200 },
    sourceSide: 'left',
    controllerBody: body,
    controllerBounds: bounds,
  })
  for (let index = 1; index < route.points.length; index += 1) {
    const previous = route.points[index - 1]
    const current = route.points[index]
    assert.ok(previous.x === current.x || previous.y === current.y)
  }
})

test('routePhysicalEdge routes opposite-side connections outside controller body', () => {
  const route = routePhysicalEdge({
    source: { x: 20, y: 120 },
    target: { x: 620, y: 200 },
    sourceSide: 'left',
    controllerBody: body,
    controllerBounds: bounds,
    gutter: 24,
  })
  assert.ok(route.points.some(point => point.y < body.y || point.y > body.y + body.height))
  assert.equal(route.path.includes('C'), false)
  assert.equal(route.path.includes('Q'), true)
})

test('routePhysicalEdge keeps same-side connections in their outside gutter', () => {
  const route = routePhysicalEdge({
    source: { x: 20, y: 120 },
    target: { x: -180, y: 280 },
    sourceSide: 'left',
    controllerBody: body,
    controllerBounds: bounds,
  })
  assert.deepEqual(route.points[1], { x: -4, y: 120 })
  assert.deepEqual(route.points[2], { x: -4, y: 280 })
})

test('routePhysicalEdge always leaves a GPIO away from the controller body', () => {
  const left = routePhysicalEdge({
    source: { x: 20, y: 120 },
    target: { x: 620, y: 200 },
    sourceSide: 'left',
    controllerBody: body,
    controllerBounds: bounds,
  })
  const right = routePhysicalEdge({
    source: { x: 500, y: 120 },
    target: { x: -120, y: 200 },
    sourceSide: 'right',
    controllerBody: body,
    controllerBounds: bounds,
  })
  assert.ok(left.points[1].x < left.points[0].x)
  assert.ok(right.points[1].x > right.points[0].x)
})

test('routePhysicalEdge assigns independent lanes to different connections', () => {
  const first = routePhysicalEdge({
    source: { x: 75, y: 120 },
    target: { x: -180, y: 280 },
    sourceSide: 'left',
    controllerBody: body,
    controllerBounds: bounds,
    laneOffset: 0,
  })
  const second = routePhysicalEdge({
    source: { x: 75, y: 142 },
    target: { x: -180, y: 320 },
    sourceSide: 'left',
    controllerBody: body,
    controllerBounds: bounds,
    laneOffset: 4,
  })
  assert.notEqual(first.points[1].x, second.points[1].x)
  assert.notEqual(first.points[2].x, second.points[2].x)
})

test('routePhysicalEdge rounds ordinary turns without marking them as junctions', () => {
  const route = routePhysicalEdge({
    source: { x: 75, y: 120 },
    target: { x: -180, y: 280 },
    sourceSide: 'left',
    controllerBody: body,
    controllerBounds: bounds,
  })
  assert.equal(route.path, 'M 75 120 L 56 120 Q 51 120 51 125 L 51 275 Q 51 280 46 280 L -180 280')
  assert.equal('junction' in route, false)
})
