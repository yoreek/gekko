import assert from 'node:assert/strict'
import test from 'node:test'

import type { MqttSettingsRecord, MqttStatusResponse } from '../../../src/api/contracts.ts'
import {
  applyMqttSettingsToStatus,
  buildMqttSettingsPayload,
  createMqttSettingsDraft,
  areMqttSettingFieldsDirty,
  isMqttSettingsDirty,
  planMqttSaveOperations,
  type MqttSettingsSnapshot,
} from '../../../src/models/mqtt-settings-form.ts'

function makeStatus(overrides: Partial<MqttStatusResponse> = {}): MqttStatusResponse {
  return {
    enabled: true,
    connected: false,
    waitingForStation: false,
    host: 'broker.local',
    port: 1883,
    useTls: false,
    clientId: 'gekko',
    hasCaCert: false,
    ...overrides,
  }
}

function makeSettings(overrides: Partial<MqttSettingsRecord> = {}): MqttSettingsRecord {
  return {
    enabled: true,
    host: 'broker.local',
    port: 1883,
    useTls: false,
    clientId: 'gekko',
    username: 'user',
    password: '',
    haDiscoveryPrefix: 'homeassistant',
    haNodeId: 'gekko',
    haNodeName: 'Gekko',
    ...overrides,
  }
}

function makeSnapshot(settings: MqttSettingsRecord | null = makeSettings()): MqttSettingsSnapshot {
  return {
    status: makeStatus(),
    settings,
  }
}

test('unavailable MQTT produces a non-editable draft without broker defaults', () => {
  const draft = createMqttSettingsDraft(makeSnapshot(null))

  assert.deepEqual(draft, { available: false })
  assert.equal(isMqttSettingsDirty(makeSnapshot(null), draft), false)
  assert.deepEqual(buildMqttSettingsPayload(draft), {})
})

test('draft is populated only from the settings API snapshot and starts pristine', () => {
  const snapshot = makeSnapshot(makeSettings({ host: 'api-broker', port: 8883 }))
  const draft = createMqttSettingsDraft(snapshot)

  assert.equal(draft.available, true)
  if (!draft.available) return
  assert.equal(draft.host, 'api-broker')
  assert.equal(draft.port, 8883)
  assert.equal(draft.password, '')
  assert.equal(draft.caCertAction, 'keep')
  assert.equal(draft.caCertFile, null)
  assert.equal(isMqttSettingsDirty(snapshot, draft), false)
})

test('staged certificate replacement and removal participate in the common dirty state', () => {
  const snapshot = makeSnapshot()
  const draft = createMqttSettingsDraft(snapshot)
  if (!draft.available) return

  draft.caCertAction = 'replace'
  draft.caCertFile = {} as File
  assert.equal(areMqttSettingFieldsDirty(snapshot, draft), false)
  assert.equal(isMqttSettingsDirty(snapshot, draft), true)

  draft.caCertAction = 'remove'
  draft.caCertFile = null
  assert.equal(isMqttSettingsDirty(snapshot, draft), true)
})

test('save plan sends settings before the separate certificate request', () => {
  const snapshot = makeSnapshot()
  const draft = createMqttSettingsDraft(snapshot)
  if (!draft.available) return

  draft.host = 'changed-broker'
  draft.caCertAction = 'replace'
  draft.caCertFile = {} as File
  assert.deepEqual(planMqttSaveOperations(snapshot, draft), ['settings', 'replace-certificate'])

  draft.host = snapshot.settings!.host
  assert.deepEqual(planMqttSaveOperations(snapshot, draft), ['replace-certificate'])

  draft.caCertAction = 'remove'
  draft.caCertFile = null
  assert.deepEqual(planMqttSaveOperations(snapshot, draft), ['remove-certificate'])
})

test('settings changes and a replacement password make the draft dirty', () => {
  const snapshot = makeSnapshot()
  const draft = createMqttSettingsDraft(snapshot)
  if (!draft.available) return

  draft.host = 'changed-broker'
  assert.equal(isMqttSettingsDirty(snapshot, draft), true)

  draft.host = snapshot.settings!.host
  draft.password = 'replacement-secret'
  assert.equal(isMqttSettingsDirty(snapshot, draft), true)
})

test('blank password is omitted while a replacement password is included in the save payload', () => {
  const draft = createMqttSettingsDraft(makeSnapshot())
  if (!draft.available) return

  assert.equal('password' in buildMqttSettingsPayload(draft), false)

  draft.password = 'replacement-secret'
  assert.equal(buildMqttSettingsPayload(draft).password, 'replacement-secret')
})

test('saved settings update duplicated status fields without losing runtime or certificate state', () => {
  const current = makeStatus({ connected: true, hasCaCert: false })
  const settings = makeSettings({ host: 'saved-broker', port: 8883, useTls: true })
  const next = applyMqttSettingsToStatus(current, settings, true)

  assert.equal(next.connected, true)
  assert.equal(next.host, 'saved-broker')
  assert.equal(next.port, 8883)
  assert.equal(next.useTls, true)
  assert.equal(next.hasCaCert, true)
})
