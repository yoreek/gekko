import assert from 'node:assert/strict'
import test from 'node:test'

import {
  calculateAnalogDaylightWindow,
  fallbackSunriseMinute,
  fallbackSunsetMinute,
  normalizeDaylightCoordinates,
} from '../../../../src/models/devices/analog-daylight.ts'

test('analog daylight calculation produces equatorial sunrise and sunset', () => {
  const result = calculateAnalogDaylightWindow(
    new Date('2026-03-20T00:00:00Z'),
    { latitude: 0, longitude: 0 },
    0,
  )

  assert.equal(result.approximate, false)
  assert.ok(result.sunriseMinute >= 350 && result.sunriseMinute <= 380)
  assert.ok(result.sunsetMinute >= 1070 && result.sunsetMinute <= 1100)
})

test('analog daylight calculation applies longitude and timezone offset', () => {
  const result = calculateAnalogDaylightWindow(
    new Date('2026-07-18T00:00:00Z'),
    { latitude: -8.65, longitude: 115.2 },
    8 * 60,
  )

  assert.equal(result.approximate, false)
  assert.ok(result.sunriseMinute >= 360 && result.sunriseMinute <= 410)
  assert.ok(result.sunsetMinute >= 1070 && result.sunsetMinute <= 1120)
})

test('analog daylight uses a stable fallback without valid coordinates', () => {
  assert.deepEqual(
    calculateAnalogDaylightWindow(new Date('2026-07-18T00:00:00Z'), null, 0),
    {
      sunriseMinute: fallbackSunriseMinute,
      sunsetMinute: fallbackSunsetMinute,
      approximate: true,
    },
  )
  assert.equal(normalizeDaylightCoordinates({ latitude: 91, longitude: 0 }), null)
  assert.deepEqual(
    normalizeDaylightCoordinates({ latitude: '-8.65', longitude: '115.2' }),
    { latitude: -8.65, longitude: 115.2 },
  )
})
