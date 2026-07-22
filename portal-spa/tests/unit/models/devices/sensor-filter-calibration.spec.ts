import assert from 'node:assert/strict'
import test from 'node:test'

import {
  rawFromDisplayed,
  roundCoefficients,
  solveOffsetOnly,
  solveTwoPoint,
} from '../../../../src/models/devices/sensor-filter-calibration.ts'

const identity = { calibrationFactor: 1, calibrationOffset: 0 }

test('rawFromDisplayed inverts the applied factor and offset', () => {
  // displayed = raw * 2 + 3  =>  raw = (displayed - 3) / 2
  assert.equal(rawFromDisplayed(23, { calibrationFactor: 2, calibrationOffset: 3 }), 10)
  assert.equal(rawFromDisplayed(5, identity), 5)
})

test('solveOffsetOnly shifts offset so displayed reads real, keeping factor', () => {
  const result = solveOffsetOnly({ displayed: 21.5, real: 22 }, identity)
  assert.equal(result.calibrationFactor, 1)
  assert.ok(Math.abs(result.calibrationOffset - 0.5) < 1e-9)
})

test('solveOffsetOnly composes with an existing offset', () => {
  const current = { calibrationFactor: 1, calibrationOffset: 2 }
  const result = solveOffsetOnly({ displayed: 25, real: 24 }, current)
  // new offset should be oldOffset + (real - displayed) = 2 + (24 - 25) = 1
  assert.ok(Math.abs(result.calibrationOffset - 1) < 1e-9)
  // and applying it: raw was (25-2)=23, now 23 + 1 = 24 = real
  assert.ok(Math.abs(23 * result.calibrationFactor + result.calibrationOffset - 24) < 1e-9)
})

test('solveTwoPoint recovers slope and offset from an identity-calibrated sensor', () => {
  // raw readings 0 and 100 currently show as 0 and 100; true values are 10 and 110 (offset 10, slope 1)
  const result = solveTwoPoint({ displayed: 0, real: 10 }, { displayed: 100, real: 110 }, identity)
  assert.ok(result)
  assert.ok(Math.abs(result!.calibrationFactor - 1) < 1e-9)
  assert.ok(Math.abs(result!.calibrationOffset - 10) < 1e-9)
})

test('solveTwoPoint corrects slope error', () => {
  // sensor reads 0->0 and 50->100 (raw), but truth is 0 and 200 => factor 2
  const result = solveTwoPoint({ displayed: 0, real: 0 }, { displayed: 50, real: 200 }, identity)
  assert.ok(result)
  assert.ok(Math.abs(result!.calibrationFactor - 4) < 1e-9)
  assert.ok(Math.abs(result!.calibrationOffset - 0) < 1e-9)
})

test('solveTwoPoint composes with existing coefficients', () => {
  // current filter already applies factor 2, offset 5: displayed = raw*2 + 5
  const current = { calibrationFactor: 2, calibrationOffset: 5 }
  // two displayed points 5 (raw 0) and 25 (raw 10); user says true temps are 0 and 20 => factor 2, offset 0
  const result = solveTwoPoint({ displayed: 5, real: 0 }, { displayed: 25, real: 20 }, current)
  assert.ok(result)
  assert.ok(Math.abs(result!.calibrationFactor - 2) < 1e-9)
  assert.ok(Math.abs(result!.calibrationOffset - 0) < 1e-9)
})

test('solveTwoPoint returns null on degenerate points', () => {
  assert.equal(solveTwoPoint({ displayed: 10, real: 1 }, { displayed: 10, real: 2 }, identity), null)
})

test('solveTwoPoint returns null when it would zero the slope', () => {
  // equal real values => factor 0, which disables calibration - reject it
  assert.equal(solveTwoPoint({ displayed: 0, real: 5 }, { displayed: 100, real: 5 }, identity), null)
})

test('roundCoefficients trims float noise to 6 decimals', () => {
  const result = roundCoefficients({ calibrationFactor: 1.00000004, calibrationOffset: 0.4999999 })
  assert.equal(result.calibrationFactor, 1)
  assert.equal(result.calibrationOffset, 0.5)
})
