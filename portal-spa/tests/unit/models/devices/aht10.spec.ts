import assert from 'node:assert/strict'
import test from 'node:test'

import type { DeviceDependencyLink } from '../../../../src/api/contracts.ts'
import { Aht10Device } from '../../../../src/models/devices/aht10.ts'

test('aht10: configurable i2c address defaults, normalizes, and encodes', () => {
  assert.equal(Aht10Device.defaultConfig().i2cAddress, 0x38)

  const deps: DeviceDependencyLink[] = [{ role: 'i2c_bus', deviceId: 7 }]
  const normalized = Aht10Device.normalizeConfig({
    name: 'Climate',
    enabled: true,
    i2cAddress: 0x41,
  }, deps)

  assert.equal(normalized.dependencyDeviceId, 7)
  assert.equal(normalized.i2cAddress, 0x41)
  assert.equal(Aht10Device.encodeConfig(normalized).i2cAddress, 0x41)
})

test('aht10: missing i2c address migrates to the hardware default', () => {
  const normalized = Aht10Device.normalizeConfig({ name: 'Legacy Climate', enabled: true }, 9)
  assert.equal(normalized.dependencyDeviceId, 9)
  assert.equal(normalized.i2cAddress, 0x38)
})

test('aht10: create payload includes configured address and bus dependency', () => {
  const device = new Aht10Device()
  const draft = device.createDefaultCreateDraft()
  draft.dependencyDeviceId = 11
  draft.i2cAddress = 0x42

  const payload = device.buildCreatePayload(draft)
  assert.equal(payload.config.i2cAddress, 0x42)
  assert.deepEqual(payload.config.deps, [{ role: 'i2c_bus', deviceId: 11 }])
})
