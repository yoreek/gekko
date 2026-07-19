import assert from 'node:assert/strict'
import test from 'node:test'

import type { DeviceDependencyLink } from '../../../../src/api/contracts.ts'
import { Pcf8574ExpanderDevice } from '../../../../src/models/devices/pcf8574-expander.ts'
import { Pcf8575ExpanderDevice } from '../../../../src/models/devices/pcf8575-expander.ts'
import {
  defaultPcf857xExpanderConfig,
  normalizePcf857xExpanderConfig,
  encodePcf857xExpanderConfig,
} from '../../../../src/models/devices/pcf857x-expander.ts'

test('pcf857x: subclasses keep their distinct identities', () => {
  assert.equal(Pcf8574ExpanderDevice.TYPE_ID, 12)
  assert.equal(Pcf8574ExpanderDevice.TYPE_NAME, 'pcf8574_expander')
  assert.equal(Pcf8574ExpanderDevice.CHANNEL_COUNT, 8)
  assert.equal(Pcf8575ExpanderDevice.TYPE_ID, 13)
  assert.equal(Pcf8575ExpanderDevice.TYPE_NAME, 'pcf8575_expander')
  assert.equal(Pcf8575ExpanderDevice.CHANNEL_COUNT, 16)

  const pcf8574 = new Pcf8574ExpanderDevice()
  const pcf8575 = new Pcf8575ExpanderDevice()
  assert.equal(pcf8574.typeName, 'pcf8574_expander')
  assert.equal(pcf8575.typeName, 'pcf8575_expander')
  assert.deepEqual(pcf8574.dependencyRoles, ['port_expander'])
  assert.deepEqual(pcf8575.dependencyRoles, ['port_expander'])
})

test('pcf857x: normalize + encode round-trips config fields', () => {
  const deps: DeviceDependencyLink[] = [{ role: 'i2c_bus', deviceId: 7 }]
  const normalized = normalizePcf857xExpanderConfig(
    { name: 'Expander', enabled: false, i2cAddress: 0x21, inverted: true },
    deps,
  )
  assert.equal(normalized.dependencyDeviceId, 7)
  assert.equal(normalized.i2cAddress, 0x21)
  assert.equal(normalized.inverted, true)

  const encoded = encodePcf857xExpanderConfig(normalized)
  assert.equal(encoded.name, 'Expander')
  assert.equal(encoded.enabled, false)
  assert.equal(encoded.i2cAddress, 0x21)
  assert.equal(encoded.inverted, true)
})

test('pcf857x: normalize falls back to defaults on garbage input', () => {
  const defaults = defaultPcf857xExpanderConfig()
  const normalized = normalizePcf857xExpanderConfig(null)
  assert.equal(normalized.i2cAddress, defaults.i2cAddress)
  assert.equal(normalized.inverted, defaults.inverted)
  assert.equal(normalized.dependencyDeviceId, 0)
})

test('pcf857x: create payload declares the i2c_bus dependency under both type names', () => {
  for (const device of [new Pcf8574ExpanderDevice(), new Pcf8575ExpanderDevice()]) {
    const draft = device.createDefaultCreateDraft()
    draft.dependencyDeviceId = 3
    const payload = device.buildCreatePayload(draft)
    assert.equal(payload.typeName, device.typeName)
    assert.deepEqual(payload.config.deps, [{ role: 'i2c_bus', deviceId: 3 }])
  }
})
