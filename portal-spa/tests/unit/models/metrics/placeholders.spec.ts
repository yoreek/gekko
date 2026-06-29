import assert from 'node:assert/strict'
import test from 'node:test'

import type { MetricPlaceholderDescriptor } from '../../../../src/api/contracts.ts'
import {
  hasInvalidMetricPlaceholders,
  metricPlaceholderForDescriptor,
  parseMetricPlaceholder,
  resolveMetricPlaceholderText,
  validateMetricPlaceholders,
} from '../../../../src/models/metrics/placeholders.ts'

const catalog: MetricPlaceholderDescriptor[] = [
  {
    placeholder: '{{dev.12.temperature}}',
    namespace: 'dev',
    sourceId: 12,
    metricId: 100,
    metricKey: 'temperature',
    label: 'Kitchen temperature',
    valueType: 'float',
    available: true,
    preview: '21.5 C',
  },
  {
    placeholder: '{{system.wifi.station_ip}}',
    namespace: 'system',
    sourceId: 0,
    metricId: 3,
    metricKey: 'wifi.station_ip',
    label: 'Station IP',
    valueType: 'string',
    available: false,
  },
]

test('parses device and global metric placeholders', () => {
  assert.deepEqual(parseMetricPlaceholder(' dev.12.temperature '), {
    raw: '{{dev.12.temperature}}',
    namespace: 'dev',
    sourceId: 12,
    metricKey: 'temperature',
    filter: null,
  })
  assert.deepEqual(parseMetricPlaceholder('system.wifi.station_ip'), {
    raw: '{{system.wifi.station_ip}}',
    namespace: 'system',
    sourceId: 0,
    metricKey: 'wifi.station_ip',
    filter: null,
  })
  assert.deepEqual(parseMetricPlaceholder(' dev.12.temperature | upper '), {
    raw: '{{dev.12.temperature}}',
    namespace: 'dev',
    sourceId: 12,
    metricKey: 'temperature',
    filter: 'upper',
  })
  assert.deepEqual(parseMetricPlaceholder(' dev.12.temperature | trim '), {
    raw: '{{dev.12.temperature}}',
    namespace: 'dev',
    sourceId: 12,
    metricKey: 'temperature',
    filter: 'trim',
  })
  assert.deepEqual(parseMetricPlaceholder(' dev.12.temperature | text '), {
    raw: '{{dev.12.temperature}}',
    namespace: 'dev',
    sourceId: 12,
    metricKey: 'temperature',
    filter: 'text',
  })
  assert.deepEqual(parseMetricPlaceholder(' dev.12.temperature |   '), {
    raw: '{{dev.12.temperature}}',
    namespace: 'dev',
    sourceId: 12,
    metricKey: 'temperature',
    filter: null,
  })
  assert.equal(parseMetricPlaceholder('dev.0.temperature'), null)
  assert.equal(parseMetricPlaceholder('system.1.time'), null)
  assert.equal(parseMetricPlaceholder('wifi.station_ip | nope'), null)
})

test('generates normalized placeholders from descriptors', () => {
  assert.equal(metricPlaceholderForDescriptor(catalog[0]), '{{dev.12.temperature}}')
  assert.equal(metricPlaceholderForDescriptor(catalog[1]), '{{system.wifi.station_ip}}')
})

test('validates static, valid, unavailable, and invalid placeholders', () => {
  assert.equal(validateMetricPlaceholders('Static label', catalog).status, 'static')
  assert.equal(validateMetricPlaceholders('Temp {{dev.12.temperature}}', catalog).status, 'valid')
  assert.equal(validateMetricPlaceholders('Temp {{dev.12.temperature | upper}}', catalog).status, 'valid')
  assert.equal(validateMetricPlaceholders('Temp {{dev.12.temperature | trim}}', catalog).status, 'valid')
  assert.equal(validateMetricPlaceholders('{{system.wifi.station_ip}}', catalog).status, 'unavailable')
  assert.equal(validateMetricPlaceholders('{{dev.nope.temperature}}', catalog).status, 'invalid')
  assert.equal(validateMetricPlaceholders('{{dev.12.temperature}} {{system.wifi.station_ip}}', catalog).status, 'unavailable')
  assert.equal(validateMetricPlaceholders('{{dev.12.temperature}} and {{dev.12.temperature}}', catalog).status, 'valid')
  assert.equal(validateMetricPlaceholders('{{dev.12.temperature}} }}', catalog).status, 'invalid')
  assert.equal(validateMetricPlaceholders('{{dev.12.temperature | nope}}', catalog).status, 'invalid')
})

test('resolves placeholder previews without changing invalid text', () => {
  assert.equal(resolveMetricPlaceholderText('Temp {{dev.12.temperature}}', catalog), 'Temp 21.5 C')
  assert.equal(resolveMetricPlaceholderText('Temp {{dev.12.temperature}} / {{dev.12.temperature}}', catalog), 'Temp 21.5 C / 21.5 C')
  assert.equal(resolveMetricPlaceholderText('Temp {{dev.12.temperature | upper}}', catalog), 'Temp 21.5 C')
  assert.equal(resolveMetricPlaceholderText('Temp {{dev.12.temperature | trim}}', catalog), 'Temp 21.5 C')
  assert.equal(resolveMetricPlaceholderText('Temp {{dev.12.temperature | text}}', catalog), 'Temp 21.5 C')
  assert.equal(resolveMetricPlaceholderText('IP {{system.wifi.station_ip}}', catalog), 'IP {{system.wifi.station_ip}}')
  assert.equal(resolveMetricPlaceholderText('{{dev.99.temperature}}', catalog), '{{dev.99.temperature}}')
  assert.equal(resolveMetricPlaceholderText('{{dev.nope.temperature', catalog), '{{dev.nope.temperature')
})

test('save-time validation rejects malformed or multiple placeholders only', () => {
  assert.equal(hasInvalidMetricPlaceholders('{{dev.nope.temperature}}'), true)
  assert.equal(hasInvalidMetricPlaceholders('{{dev.12.temperature}} {{system.wifi.station_ip}}'), false)
  assert.equal(hasInvalidMetricPlaceholders('{{dev.12.temperature}}'), false)
  assert.equal(hasInvalidMetricPlaceholders('{{dev.12.temperature | upper}}'), false)
  assert.equal(hasInvalidMetricPlaceholders('{{dev.12.temperature}} }}'), true)
})
