import assert from 'node:assert/strict'
import test from 'node:test'

import {
  deviceActionPresets,
  normalizeDeviceCollection,
  normalizeDeviceDetail,
  normalizeDeviceRecord,
} from '../src/models/device-model.ts'
import type { DeviceRecord, DeviceRegistryResponse, DeviceDetailResponse, DeviceMutationResponse } from '../src/api/contracts.ts'

const dummyRecord: DeviceRecord = {
  device_id: 101,
  type_id: 1,
  type: 'dummy',
  name: 'Aquarium Lamp',
  enabled: true,
  has_parent: false,
  parent_device_id: 0,
  config_version: 2,
  config_revision: 4,
  lifecycle_status: 'ready',
  effective_status: 'ready',
  status: 'ready',
  persistence_policy: 'delayed',
  retained_state_supported: true,
  retained_startup_enabled: true,
  retained_startup_fallback_output: false,
  retained_state_in_config_payload: false,
  config: {
    enabled: true,
    restore_previous_state: true,
    default_output: false,
    current_output: true,
  },
}

test('normalizeDeviceRecord maps the shared fields and typed dummy detail', () => {
  const device = normalizeDeviceRecord(dummyRecord, 12, true)

  assert.equal(device.deviceId, 101)
  assert.equal(device.typeName, 'dummy')
  assert.equal(device.typeLabel, 'DummyDevice')
  assert.equal(device.kind, 'dummy')
  assert.equal(device.registryRevision, 12)
  assert.equal(device.pendingPersistence, true)
  assert.equal(device.status, 'ready')
  assert.equal(device.detail.outputState, true)
  assert.equal(device.detail.restorePreviousState, true)
  assert.equal(device.detail.retainedStateSupported, true)
  assert.equal(device.detail.config.current_output, true)
})

test('normalizeDeviceCollection keeps the registry envelope and typed devices', () => {
  const payload: DeviceRegistryResponse = {
    registry_revision: 8,
    pending_persistence: false,
    devices: [dummyRecord],
    success: true,
  }

  const collection = normalizeDeviceCollection(payload)
  assert.equal(collection.registryRevision, 8)
  assert.equal(collection.pendingPersistence, false)
  assert.equal(collection.devices.length, 1)
  assert.equal(collection.devices[0].typeLabel, 'DummyDevice')
})

test('normalizeDeviceDetail returns the device from REST envelopes and null when missing', () => {
  const detail: DeviceDetailResponse = {
    registry_revision: 9,
    pending_persistence: true,
    device: dummyRecord,
    success: true,
  }
  const mutation: DeviceMutationResponse = {
    registry_revision: 10,
    pending_persistence: false,
    success: true,
  }

  assert.equal(normalizeDeviceDetail(detail)?.registryRevision, 9)
  assert.equal(normalizeDeviceDetail(mutation), null)
})

test('deviceActionPresets returns typed DummyDevice quick commands only for dummy devices', () => {
  const dummyDevice = normalizeDeviceRecord(dummyRecord)
  const genericDevice = normalizeDeviceRecord({
    ...dummyRecord,
    device_id: 202,
    type_id: 7,
    type: 'generic',
    name: 'Unknown Sensor',
  })

  const presets = deviceActionPresets(dummyDevice)
  assert.equal(presets.length, 4)
  assert.equal(presets[0].command, 'custom')
  assert.equal(presets[0].labelKey, 'device.commands.outputOn')
  assert.equal(deviceActionPresets(genericDevice).length, 0)
})
