import assert from 'node:assert/strict'
import test from 'node:test'

import type { BaseDeviceConfig, DeviceRecord } from '../../../../src/api/contracts.ts'
import { buildDeviceDependencyGraph, layoutDeviceDependencyGraph } from '../../../../src/models/devices/dependency-graph.ts'

function device(id: number, deps: BaseDeviceConfig['deps'] = []): DeviceRecord {
  return {
    record: { id, typeName: 'dummy', configRevision: 1 },
    config: { name: `Device ${id}`, enabled: true, deps },
    runtime: { status: 'ok', lifecycleStatus: 'ready', effectiveStatus: 'ok' },
  }
}

test('buildDeviceDependencyGraph includes isolated nodes and directs dependencies to dependents', () => {
  const graph = buildDeviceDependencyGraph([
    device(2, [{ role: 'switch', deviceId: 1 }]),
    device(1),
    device(3),
  ])
  assert.deepEqual(graph.nodes.map(node => node.id), ['1', '2', '3'])
  assert.deepEqual(graph.edges, [{ id: '1->2:switch:0', source: '1', target: '2', role: 'switch', invert: false }])
  assert.deepEqual(graph.diagnostics, [])
})

test('graph reports dangling dependencies without creating phantom nodes', () => {
  const graph = buildDeviceDependencyGraph([device(4, [{ role: 'condition', deviceId: 99, invert: true }])])
  assert.deepEqual(graph.nodes.map(node => node.id), ['4'])
  assert.deepEqual(graph.edges, [])
  assert.deepEqual(graph.diagnostics, [{ kind: 'danglingDependency', dependentDeviceId: 4, dependencyDeviceId: 99, role: 'condition', invert: true }])
})

test('topologyKey is stable across registry ordering and runtime changes', () => {
  const first = device(2, [{ role: 'switch', deviceId: 1 }])
  const second = device(1)
  const a = buildDeviceDependencyGraph([first, second])
  const changedRuntime = { ...first, runtime: { ...first.runtime, status: 'warning' } }
  const b = buildDeviceDependencyGraph([second, changedRuntime])
  assert.equal(a.topologyKey, b.topologyKey)
  assert.notEqual(buildDeviceDependencyGraph([device(2)]).topologyKey, a.topologyKey)
})

test('layoutDeviceDependencyGraph returns deterministic non-overlapping positions', () => {
  const graph = buildDeviceDependencyGraph([
    device(1),
    device(2, [{ role: 'switch', deviceId: 1 }]),
    device(3, [{ role: 'switch', deviceId: 1 }]),
  ])
  const dimensions = { '1': { width: 100, height: 50 }, '2': { width: 120, height: 60 }, '3': { width: 120, height: 60 } }
  const positions = layoutDeviceDependencyGraph(graph, dimensions, { direction: 'LR' })
  assert.deepEqual(Object.keys(positions).sort(), ['1', '2', '3'])
  assert.ok(positions['2'].x > positions['1'].x)
  assert.ok(positions['3'].x > positions['1'].x)
  assert.notDeepEqual(positions['2'], positions['3'])
  assert.deepEqual(positions, layoutDeviceDependencyGraph(graph, dimensions, { direction: 'LR' }))
})
