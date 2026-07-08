import assert from 'node:assert/strict'
import test from 'node:test'

import type { BaseDeviceConfig, DeviceRecord } from '../../../../src/api/contracts.ts'
import { Ds18b20Device } from '../../../../src/models/devices/ds18b20.ts'
import { NtcThermistorDevice } from '../../../../src/models/devices/ntc-thermistor.ts'
import { GpioSwitchDevice } from '../../../../src/models/devices/gpio-switch.ts'
import { devicesForDependencyRole, dependencyOptionsForRole } from '../../../../src/models/devices/device-model-factory.ts'

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
