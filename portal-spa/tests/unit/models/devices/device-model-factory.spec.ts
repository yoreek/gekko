import assert from 'node:assert/strict'
import test from 'node:test'

import type { BaseDeviceConfig, DeviceRecord } from '../../../../src/api/contracts.ts'
import { Ds18b20Device } from '../../../../src/models/devices/ds18b20.ts'
import { NtcThermistorDevice } from '../../../../src/models/devices/ntc-thermistor.ts'
import { GpioSwitchDevice } from '../../../../src/models/devices/gpio-switch.ts'
import { AnalogOutputDevice } from '../../../../src/models/devices/analog-output.ts'
import {
  FadeAnalogOutputDevice,
  ScheduledAnalogOutputDevice,
} from '../../../../src/models/devices/composable-analog-output.ts'
import {
  devicesForDependencyRole,
  dependencyOptionsForRole,
  exclusiveAnalogOutputDependencyOptions,
} from '../../../../src/models/devices/device-model-factory.ts'

function makeRecord(id: number, typeName: string, name: string, config: BaseDeviceConfig): DeviceRecord {
  return {
    record: { id, typeName, configRevision: 1 },
    config: { ...config, name },
    runtime: { status: 'ok', lifecycleStatus: 'ready', effectiveStatus: 'ok' },
  }
}

test('devicesForDependencyRole: temperature_sensor matches every temperature-sensor type, not just one hardcoded type id', () => {
  const devices = [
    makeRecord(1, Ds18b20Device.TYPE_NAME, 'DS18B20 sensor', Ds18b20Device.defaultConfig()),
    makeRecord(2, NtcThermistorDevice.TYPE_NAME, 'NTC sensor', NtcThermistorDevice.defaultConfig()),
    makeRecord(3, GpioSwitchDevice.TYPE_NAME, 'Relay', GpioSwitchDevice.defaultConfig()),
  ]

  const sensors = devicesForDependencyRole(devices, 'temperature_sensor')
  assert.deepEqual(sensors.map(d => d.record.id), [1, 2])

  const switches = devicesForDependencyRole(devices, 'switch')
  assert.deepEqual(switches.map(d => d.record.id), [3])
})

test('dependencyOptionsForRole: builds "name #id" v-select options for the matching role only', () => {
  const devices = [
    makeRecord(1, Ds18b20Device.TYPE_NAME, 'DS18B20 sensor', Ds18b20Device.defaultConfig()),
    makeRecord(3, GpioSwitchDevice.TYPE_NAME, 'Relay', GpioSwitchDevice.defaultConfig()),
  ]

  assert.deepEqual(dependencyOptionsForRole(devices, 'temperature_sensor'), [{ title: 'DS18B20 sensor #1', value: 1 }])
  assert.deepEqual(dependencyOptionsForRole(devices, 'switch'), [{ title: 'Relay #3', value: 3 }])
  assert.deepEqual(dependencyOptionsForRole(devices, 'i2c_bus'), [])
})

test('analog output model uses one flat output config', () => {
  const model = new AnalogOutputDevice()
  const draft = model.createDefaultCreateDraft({
    name: 'PWM output',
    startupState: 35,
    safeState: 10,
    pin: 12,
  })
  const payload = model.buildCreatePayload(draft)

  assert.equal(payload.typeName, 'analog_output')
  assert.equal(payload.config.startupState, 35)
  assert.equal(payload.config.safeState, 10)
  assert.equal(payload.config.pin, 12)
  assert.equal('channelCount' in payload.config, false)
  assert.equal('channels' in payload.config, false)
})

test('composable analog output models encode scalar dependencies and schedule points', () => {
  const fade = new FadeAnalogOutputDevice()
  const fadePayload = fade.buildCreatePayload({
    ...fade.createDefaultCreateDraft({ name: 'Fade' }),
    targetDeviceId: 11,
    maxStep: 5,
  })
  assert.deepEqual(fadePayload.config.deps, [{ role: 'analog_output', deviceId: 11 }])
  assert.equal(fadePayload.config.maxStep, 5)
  assert.equal('targetDeviceId' in fadePayload.config, false)

  const scheduled = new ScheduledAnalogOutputDevice()
  const scheduledPayload = scheduled.buildCreatePayload({
    ...scheduled.createDefaultCreateDraft({ name: 'Schedule' }),
    targetDeviceId: 12,
    points: [{ deleted: false, minuteOfDay: 720, state: 80 }],
  })
  assert.deepEqual(scheduledPayload.config.deps, [{ role: 'analog_output', deviceId: 12 }])
  assert.deepEqual(scheduledPayload.config.points, [{ deleted: false, minuteOfDay: 720, state: 80 }])
})

test('exclusive analog output options hide targets owned by another controller', () => {
  const firstOutput = makeRecord(1, AnalogOutputDevice.TYPE_NAME, 'First', AnalogOutputDevice.defaultConfig())
  const secondOutput = makeRecord(2, AnalogOutputDevice.TYPE_NAME, 'Second', AnalogOutputDevice.defaultConfig())
  const fadeConfig = new FadeAnalogOutputDevice().createDefaultConfig()
  const fade = makeRecord(10, FadeAnalogOutputDevice.TYPE_NAME, 'Fade', {
    ...fadeConfig,
    deps: [{ role: 'analog_output', deviceId: 1 }],
  })
  const devices = [firstOutput, secondOutput, fade]

  assert.deepEqual(exclusiveAnalogOutputDependencyOptions(devices), [
    { title: 'Second #2', value: 2 },
    { title: 'Fade #10', value: 10 },
  ])
  assert.deepEqual(exclusiveAnalogOutputDependencyOptions(devices, 10), [
    { title: 'First #1', value: 1 },
    { title: 'Second #2', value: 2 },
  ])
})
