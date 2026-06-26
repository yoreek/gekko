import assert from 'node:assert/strict'
import test from 'node:test'

import { createPinia, setActivePinia } from 'pinia'

import { useDeviceEventLogStore } from '../../src/stores/deviceEventLog.ts'
import type { RealtimeMessage } from '../../src/realtime/messages.ts'

function createStore() {
  const pinia = createPinia()
  setActivePinia(pinia)
  return useDeviceEventLogStore(pinia)
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function makeSnapshotMessage(
  eventKind: string,
  deviceId = 11,
  overrides: Record<string, unknown> = {},
): RealtimeMessage<Record<string, unknown>> {
  const recordOverrides = isRecord(overrides.record) ? overrides.record : {}
  const configOverrides = isRecord(overrides.config) ? overrides.config : {}
  const runtimeOverrides = isRecord(overrides.runtime) ? overrides.runtime : {}
  return {
    topic: eventKind === 'command_accepted' || eventKind === 'command_rejected' ? 'device.command_result' : 'device.upsert',
    revision: 19,
    payload: {
      eventKind,
      record: {
        id: deviceId,
        typeName: typeof overrides.typeName === 'string' ? overrides.typeName : 'gpio_switch',
        configRevision: typeof overrides.configRevision === 'number' ? overrides.configRevision : 3,
        ...recordOverrides,
      },
      config: {
        name: typeof overrides.name === 'string' ? overrides.name : `Device ${deviceId}`,
        enabled: typeof overrides.enabled === 'boolean' ? overrides.enabled : true,
        deps: Array.isArray(overrides.deps) ? overrides.deps : [],
        ...configOverrides,
      },
      runtime: {
        lifecycleStatus: typeof overrides.lifecycleStatus === 'string' ? overrides.lifecycleStatus : 'ready',
        effectiveStatus: typeof overrides.effectiveStatus === 'string' ? overrides.effectiveStatus : 'ready',
        status: typeof overrides.status === 'string' ? overrides.status : 'ready',
        ...runtimeOverrides,
      },
      registryRevision: 19,
    },
  }
}

test('classifies and stores event kinds in the journal', () => {
  const store = createStore()

  const created = store.append(makeSnapshotMessage('device_created', 21), 101)
  const updated = store.append(makeSnapshotMessage('state_changed', 22), 102)
  const command = store.append(makeSnapshotMessage('command_accepted', 23), 103)
  const snapshot = store.append(makeSnapshotMessage('snapshot', 24), 104)
  const deleted = store.append({
    topic: 'device.remove',
    revision: 20,
    payload: {
      eventKind: 'device_deleted',
      record: {
        id: 25,
        typeName: 'gpio_switch',
        configRevision: 3,
      },
      config: {
        name: 'Removed Device',
        enabled: true,
        deps: [],
      },
      runtime: {
        lifecycleStatus: 'ready',
        effectiveStatus: 'ready',
        status: 'ready',
      },
      registryRevision: 20,
    },
  }, 105)

  assert.equal(created?.eventKind, 'device_created')
  assert.equal(updated?.eventKind, 'state_changed')
  assert.equal(command?.eventKind, 'command_accepted')
  assert.equal(snapshot?.eventKind, 'snapshot')
  assert.equal(deleted?.eventKind, 'device_deleted')
  assert.equal(store.entries[0].eventKind, 'device_deleted')
  assert.equal(store.entries[0].name, 'Removed Device')
})

test('keeps the newest entries and drops the oldest when bounded', () => {
  const store = createStore()

  for (let index = 1; index <= 201; index += 1) {
    const entry = store.append(makeSnapshotMessage('device_updated', index), index)
    assert.ok(entry)
  }

  assert.equal(store.entries.length, 200)
  assert.equal(store.entries[0].deviceId, 201)
  assert.equal(store.entries.at(-1)?.deviceId, 2)
})

test('filters by type, event kind, partial name, and exact device id', () => {
  const store = createStore()

  store.append(makeSnapshotMessage('device_created', 31, { typeId: 2, typeName: 'gpio_switch', name: 'Kitchen Relay' }), 201)
  store.append(makeSnapshotMessage('state_changed', 32, { typeId: 4, typeName: 'ds18b20_temperature_sensor', name: 'Kitchen Probe' }), 202)
  store.append(makeSnapshotMessage('snapshot', 33, { typeId: 2, typeName: 'gpio_switch', name: 'Porch Light' }), 203)

  const filtered = store.filteredEntries({
    typeId: 4,
    eventKind: 'state_changed',
    name: 'kitchen',
    deviceId: '32',
  })

  assert.equal(filtered.length, 1)
  assert.equal(filtered[0].deviceId, 32)
})

test('returns the latest five events for a device in newest-first order', () => {
  const store = createStore()

  for (let index = 1; index <= 6; index += 1) {
    store.append(makeSnapshotMessage('device_updated', 77, { name: `Device ${index}` }), 300 + index)
  }
  store.append(makeSnapshotMessage('device_updated', 88, { name: 'Other device' }), 400)

  const latest = store.latestEntriesForDevice(77, 5)
  assert.equal(latest.length, 5)
  assert.equal(latest[0].name, 'Device 6')
  assert.equal(latest[4].name, 'Device 2')
})
