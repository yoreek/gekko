import assert from 'node:assert/strict'
import test from 'node:test'

import { RtcDs1302Device } from '../../../../src/models/devices/rtc-ds1302.ts'

test('rtc_ds1302: default config has no real default pins and no dependencies', () => {
  const defaults = RtcDs1302Device.defaultConfig()
  assert.equal(defaults.clkPin, 255)
  assert.equal(defaults.dataPin, 255)
  assert.equal(defaults.rstPin, 255)
  assert.equal(defaults.useForSystemTimeSync, false)
  assert.deepEqual(defaults.deps, [])
})

test('rtc_ds1302: normalizes pin fields and the sync flag from a partial payload', () => {
  const normalized = RtcDs1302Device.normalizeConfig({
    name: 'Battery RTC',
    enabled: true,
    clkPin: 14,
    dataPin: 12,
    rstPin: 13,
    useForSystemTimeSync: true,
  })

  assert.equal(normalized.clkPin, 14)
  assert.equal(normalized.dataPin, 12)
  assert.equal(normalized.rstPin, 13)
  assert.equal(normalized.useForSystemTimeSync, true)
  assert.deepEqual(normalized.deps, [])
})

test('rtc_ds1302: missing/invalid pin fields fall back to defaults', () => {
  const normalized = RtcDs1302Device.normalizeConfig({ name: 'Legacy RTC', enabled: true, clkPin: -1 })
  assert.equal(normalized.clkPin, 255)
  assert.equal(normalized.dataPin, 255)
  assert.equal(normalized.rstPin, 255)
})

test('rtc_ds1302: encodeConfig round-trips pins and never includes a dependency', () => {
  const config = RtcDs1302Device.normalizeConfig({
    name: 'Battery RTC',
    enabled: true,
    clkPin: 4,
    dataPin: 5,
    rstPin: 16,
  })
  const encoded = RtcDs1302Device.encodeConfig(config)
  assert.equal(encoded.clkPin, 4)
  assert.equal(encoded.dataPin, 5)
  assert.equal(encoded.rstPin, 16)
  assert.deepEqual(encoded.deps, [])
})

test('rtc_ds1302: create payload never carries a dependency link', () => {
  const device = new RtcDs1302Device()
  const draft = device.createDefaultCreateDraft()
  const payload = device.buildCreatePayload(draft)
  assert.deepEqual(payload.config.deps, [])
})
