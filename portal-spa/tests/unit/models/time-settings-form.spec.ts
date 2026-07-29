import assert from 'node:assert/strict'
import test from 'node:test'

import {
  buildTimeSettingsPayload,
  createTimeSettingsDraft,
  isTimeSettingsDirty,
  type TimeSettingsSnapshot,
} from '../../../src/models/time-settings-form.ts'

function createSnapshot(): TimeSettingsSnapshot {
  return {
    status: {
      enabled: true,
      synced: true,
      waitingForStation: false,
      ntpServer: 'status.example.org',
      timezoneId: 'Etc/GMT',
      syncIntervalSeconds: 1800,
      source: 'ntp',
    },
    settings: {
      enabled: false,
      ntpServer: 'settings.example.org',
      timezoneId: 'Europe/Kyiv',
      syncIntervalSeconds: 3600,
    },
    timezones: [
      { id: 'Etc/GMT', name: 'Greenwich Mean Time' },
      { id: 'Europe/Kyiv', name: 'Eastern European Time' },
    ],
  }
}

test('draft is created from the authoritative settings response instead of runtime status', () => {
  const source = createSnapshot()

  assert.deepEqual(createTimeSettingsDraft(source), {
    enabled: false,
    ntpServer: 'settings.example.org',
    timezoneId: 'Europe/Kyiv',
    syncIntervalSeconds: 3600,
  })
})

test('unchanged draft is pristine and each editable field participates in dirty state', () => {
  const source = createSnapshot()
  const draft = createTimeSettingsDraft(source)

  assert.equal(isTimeSettingsDirty(source, draft), false)

  draft.syncIntervalSeconds = 7200
  assert.equal(isTimeSettingsDirty(source, draft), true)
})

test('save payload contains only editable settings fields', () => {
  const source = createSnapshot()
  const draft = createTimeSettingsDraft(source)

  assert.deepEqual(buildTimeSettingsPayload(draft), {
    enabled: false,
    ntpServer: 'settings.example.org',
    timezoneId: 'Europe/Kyiv',
    syncIntervalSeconds: 3600,
  })
})
