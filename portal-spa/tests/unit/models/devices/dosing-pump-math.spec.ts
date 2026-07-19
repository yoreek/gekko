import assert from 'node:assert/strict'
import test from 'node:test'

import {
  calibrationSpeed,
  doseRunSeconds,
  generateDoses,
  minuteOfDayFromTime,
  speedsAreEquivalent,
  timeFromMinuteOfDay,
  todayLocalDayNumber,
  totalScheduleAmount,
} from '../../../../src/models/devices/dosing-pump-math.ts'
import { DosingPumpDevice } from '../../../../src/models/devices/dosing-pump.ts'
import type { BaseDeviceConfig, DeviceRecord } from '../../../../src/api/contracts.ts'

test('time helpers round-trip HH:mm and reject malformed values', () => {
  assert.equal(minuteOfDayFromTime('08:30'), 8 * 60 + 30)
  assert.equal(minuteOfDayFromTime('23:59'), 1439)
  assert.equal(minuteOfDayFromTime('24:00'), -1)
  assert.equal(minuteOfDayFromTime('8:5'), -1)
  assert.equal(minuteOfDayFromTime('bogus'), -1)
  assert.equal(timeFromMinuteOfDay(510), '08:30')
  assert.equal(timeFromMinuteOfDay(0), '00:00')
})

test('generator splits total into equally spaced doses and the last dose absorbs rounding', () => {
  const doses = generateDoses('09:00', '21:00', 10, 3)
  assert.ok(doses)
  assert.equal(doses.length, 3)
  assert.equal(doses[0].time, '09:00')
  assert.equal(doses[1].time, '13:00')
  assert.equal(doses[2].time, '17:00')
  // 10 / 3 = 3.333... -> 3.3 + 3.3 + 3.4 (last absorbs the remainder so the sum is exact)
  assert.equal(doses[0].amountMl, 3.3)
  assert.equal(doses[1].amountMl, 3.3)
  assert.equal(doses[2].amountMl, 3.4)
  assert.equal(totalScheduleAmount(doses), 10)
})

test('generator rejects windows too small for the dose count and non-positive totals', () => {
  assert.equal(generateDoses('09:00', '09:02', 10, 3), null)
  assert.equal(generateDoses('21:00', '09:00', 10, 3), null)
  assert.equal(generateDoses('09:00', '21:00', 0, 3), null)
  assert.equal(generateDoses('09:00', '21:00', 10, 0), null)
  assert.equal(generateDoses('09:00', '21:00', 10, 17), null)
})

test('run-time and calibration math mirror the firmware fixed-point arithmetic', () => {
  assert.equal(doseRunSeconds(5, 1), 5)
  assert.equal(doseRunSeconds(12.3, 0.125), 98.4)
  assert.equal(doseRunSeconds(5, 0), 0)
  assert.equal(calibrationSpeed(4.8, 10), 0.48)
  assert.equal(calibrationSpeed(0, 10), 0)
  assert.ok(speedsAreEquivalent(1, 1.005))
  assert.ok(!speedsAreEquivalent(1, 1.02))
})

test('todayLocalDayNumber matches manual local-midnight arithmetic', () => {
  const now = new Date(2026, 6, 13, 12, 0, 0) // local noon, 2026-07-13
  const expected = Math.floor((now.getTime() - now.getTimezoneOffset() * 60000) / 86400000)
  assert.equal(todayLocalDayNumber(now), expected)
})

function makeRecord(config: BaseDeviceConfig): DeviceRecord {
  return {
    record: { id: 7, typeName: 'dosing_pump', configRevision: 1 },
    config,
    runtime: { status: 'ready', lifecycleStatus: 'ready', effectiveStatus: 'ready' },
  }
}

test('dosing pump model: deps come from projections, not the config payload', () => {
  const model = new DosingPumpDevice()
  const draft = {
    ...DosingPumpDevice.defaultConfig(),
    name: 'Calcium',
    pumpSwitchDeviceId: 11,
    levelSensorDeviceId: 12,
    levelSensorInvert: true,
    typeName: model.typeName,
  }
  const payload = model.buildCreatePayload(draft)
  assert.deepEqual(payload.config.deps, [
    { role: 'switch', deviceId: 11 },
    { role: 'condition', deviceId: 12, invert: true },
  ])
  const configKeys = Object.keys(payload.config)
  assert.ok(!configKeys.includes('pumpSwitchDeviceId'))
  assert.ok(!configKeys.includes('levelSensorDeviceId'))
  assert.ok(!configKeys.includes('levelSensorInvert'))
})

test('dosing pump model: doses are sorted by time on the way out', () => {
  const model = new DosingPumpDevice()
  const current = DosingPumpDevice.defaultConfig()
  current.pumpSwitchDeviceId = 11
  const record = makeRecord({ ...current, deps: [{ role: 'switch', deviceId: 11 }] } as unknown as BaseDeviceConfig)
  const commands = model.buildQuickUpdateCommands(record, {
    schedule: {
      ...current.schedule,
      doses: [
        { time: '20:00', amountMl: 5 },
        { time: '08:00', amountMl: 3 },
      ],
    },
  })
  const updateConfig = commands.find(command => command.command === 'updateConfig')
  assert.ok(updateConfig)
  const schedule = updateConfig!.config?.schedule as { doses: { time: string }[] }
  assert.deepEqual(schedule.doses.map(dose => dose.time), ['08:00', '20:00'])
})

test('dosing pump model: speed-only quick update diffs a single key', () => {
  const model = new DosingPumpDevice()
  const current = DosingPumpDevice.defaultConfig()
  current.pumpSwitchDeviceId = 11
  const record = makeRecord({ ...current, deps: [{ role: 'switch', deviceId: 11 }] } as unknown as BaseDeviceConfig)
  const commands = model.buildQuickUpdateCommands(record, { dosingSpeedMlPerSec: 1.3 })
  const updateConfig = commands.find(command => command.command === 'updateConfig')
  assert.ok(updateConfig)
  assert.deepEqual(Object.keys(updateConfig!.config ?? {}), ['dosingSpeedMlPerSec'])
  assert.equal(updateConfig!.deps, undefined)
})
