import assert from 'node:assert/strict'
import test from 'node:test'

import type { DeviceRecord } from '../../../../src/api/contracts.ts'
import { extractDeviceHardwarePins } from '../../../../src/models/devices/physical-connections.ts'

function device(typeName: string, config: Record<string, unknown>): DeviceRecord {
  return {
    record: { id: 1, typeName, configRevision: 1 },
    config: { name: typeName, enabled: true, ...config },
    runtime: { status: 'ok', lifecycleStatus: 'ready', effectiveStatus: 'ok' },
  }
}

test('extractDeviceHardwarePins returns named configured GPIO connections', () => {
  assert.deepEqual(extractDeviceHardwarePins(device('i2c_bus', { sdaPin: 21, sclPin: 22 })), [
    { gpio: 21, label: 'SDA' },
    { gpio: 22, label: 'SCL' },
  ])
  assert.deepEqual(extractDeviceHardwarePins(device('spi_bus', { sckPin: 18, mosiPin: 23, misoPin: 255 })), [
    { gpio: 18, label: 'SCK' },
    { gpio: 23, label: 'MOSI' },
  ])
})
