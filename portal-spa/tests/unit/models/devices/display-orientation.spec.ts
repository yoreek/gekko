import assert from 'node:assert/strict'
import test from 'node:test'

import {
  normalizeDisplayRotation,
  resolveDisplayEffectiveSize,
  resolveDisplayOrientationGroup,
} from '../../../../src/models/devices/display/orientation.ts'

test('normalizes display rotation into the supported 0..3 range', () => {
  assert.equal(normalizeDisplayRotation(undefined, 1), 1)
  assert.equal(normalizeDisplayRotation(5), 1)
  assert.equal(normalizeDisplayRotation(-1), 3)
})

test('maps display rotation to the logical orientation group', () => {
  assert.equal(resolveDisplayOrientationGroup(128, 64, 0), 'landscape')
  assert.equal(resolveDisplayOrientationGroup(128, 64, 1), 'portrait')
  assert.equal(resolveDisplayOrientationGroup(128, 64, 2), 'landscape')
  assert.equal(resolveDisplayOrientationGroup(128, 64, 3), 'portrait')
  assert.equal(resolveDisplayOrientationGroup(128, 160, 0), 'portrait')
  assert.equal(resolveDisplayOrientationGroup(128, 160, 1), 'landscape')
  assert.equal(resolveDisplayOrientationGroup(128, 160, 2), 'portrait')
  assert.equal(resolveDisplayOrientationGroup(128, 160, 3), 'landscape')
})

test('derives effective size from physical dimensions and rotation, including 180/270', () => {
  assert.deepEqual(resolveDisplayEffectiveSize(128, 64, 0), { effectiveWidth: 128, effectiveHeight: 64 })
  assert.deepEqual(resolveDisplayEffectiveSize(128, 64, 1), { effectiveWidth: 64, effectiveHeight: 128 })
  assert.deepEqual(resolveDisplayEffectiveSize(128, 64, 2), { effectiveWidth: 128, effectiveHeight: 64 })
  assert.deepEqual(resolveDisplayEffectiveSize(128, 64, 3), { effectiveWidth: 64, effectiveHeight: 128 })
})
