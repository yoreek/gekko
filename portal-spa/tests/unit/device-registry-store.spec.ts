import assert from 'node:assert/strict'
import test from 'node:test'

import { mergeUpsertedDevice } from '../../src/stores/device-upsert-merge.ts'
import type { DeviceRecord } from '../../src/api/contracts.ts'

function makeDevice(overrides: Partial<DeviceRecord> = {}): DeviceRecord {
  return {
    record: { id: 1261905410, typeName: 'ds18b20_temperature_sensor', configRevision: 1 },
    config: { name: 'DS18B20', enabled: true, deps: [] },
    runtime: { status: 'ready', lifecycleStatus: 'ready', effectiveStatus: 'ready' },
    ...overrides,
  }
}

test('mergeUpsertedDevice preserves a previously-fetched ha field when the incoming payload omits it', () => {
  const previous = makeDevice({ ha: { supported: true, enabled: true, name: '', effectiveName: 'DS18B20' } })
  // Mirrors a routine device.upsert realtime push (e.g. a new sensor reading), which is built
  // from IDeviceApiAdapter::writeDeviceJson alone and never includes `ha`.
  const incoming = makeDevice({ runtime: { status: 'ready', lifecycleStatus: 'ready', effectiveStatus: 'ready', dependencyStatus: 'ok' } })

  const merged = mergeUpsertedDevice(previous, incoming)
  assert.deepEqual(merged.ha, { supported: true, enabled: true, name: '', effectiveName: 'DS18B20' })
  assert.deepEqual(merged.runtime, incoming.runtime)
})

test('mergeUpsertedDevice applies a fresh ha field when the incoming payload actually includes one', () => {
  const previous = makeDevice({ ha: { supported: true, enabled: false, name: '', effectiveName: 'DS18B20' } })
  // Mirrors the setHaSettings command response, which always includes an up-to-date `ha`.
  const incoming = makeDevice({ ha: { supported: true, enabled: true, name: 'Tank', effectiveName: 'Tank' } })

  const merged = mergeUpsertedDevice(previous, incoming)
  assert.deepEqual(merged.ha, { supported: true, enabled: true, name: 'Tank', effectiveName: 'Tank' })
})

test('mergeUpsertedDevice returns the incoming record as-is when there is no previous record', () => {
  const incoming = makeDevice()
  assert.equal(mergeUpsertedDevice(undefined, incoming), incoming)
})

test('mergeUpsertedDevice returns the incoming record as-is when neither side has ha', () => {
  const previous = makeDevice()
  const incoming = makeDevice({ runtime: { status: 'faulted', lifecycleStatus: 'ready', effectiveStatus: 'faulted' } })
  assert.equal(mergeUpsertedDevice(previous, incoming), incoming)
})
