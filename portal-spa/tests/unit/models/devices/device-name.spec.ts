import assert from 'node:assert/strict'
import test from 'node:test'

import {
  DEFAULT_DEVICE_NAMES,
  MAX_DEVICE_NAME_BYTES,
  deviceNameByteLength,
  isValidDeviceName,
  nextAvailableDeviceName,
} from '../../../../src/models/devices/device-name.ts'

test('device-name validation measures UTF-8 bytes instead of JavaScript code units', () => {
  assert.equal(deviceNameByteLength('я'.repeat(16)), MAX_DEVICE_NAME_BYTES)
  assert.equal(deviceNameByteLength('я'.repeat(17)), MAX_DEVICE_NAME_BYTES + 2)
  assert.equal(isValidDeviceName('я'.repeat(16)), true)
  assert.equal(isValidDeviceName('я'.repeat(17)), false)
  assert.equal(isValidDeviceName('   '), false)
})

test('every registered type has a compact valid default device name', () => {
  for (const [typeName, name] of Object.entries(DEFAULT_DEVICE_NAMES)) {
    assert.equal(isValidDeviceName(name), true, `${typeName} default name exceeds the firmware limit`)
    assert.match(name, /^[\x20-\x7E]+$/, `${typeName} default name must stay compact ASCII`)
  }
})

test('nextAvailableDeviceName adds a unique suffix within the UTF-8 byte limit', () => {
  assert.equal(nextAvailableDeviceName('DS3231', []), 'DS3231')
  assert.equal(nextAvailableDeviceName('DS3231', ['ds3231', 'DS3231 2']), 'DS3231 3')

  const longCyrillicName = 'я'.repeat(20)
  const generated = nextAvailableDeviceName(longCyrillicName, ['я'.repeat(16)])
  assert.equal(generated.endsWith(' 2'), true)
  assert.equal(deviceNameByteLength(generated) <= MAX_DEVICE_NAME_BYTES, true)
})
