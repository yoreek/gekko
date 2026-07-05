import assert from 'node:assert/strict'
import test from 'node:test'

import type { BaseDeviceConfig, DeviceRecord } from '../../../../src/api/contracts.ts'
import { GpioSwitchDevice, type GpioSwitchConfigDraft } from '../../../../src/models/devices/gpio-switch.ts'
import { Ssd1306Device, type Ssd1306ConfigDraft } from '../../../../src/models/devices/ssd1306/device.ts'
import { ThermostatDevice, type ThermostatConfigDraft } from '../../../../src/models/devices/thermostat.ts'
import { Ds18b20Device, type Ds18b20ConfigDraft } from '../../../../src/models/devices/ds18b20.ts'

function makeRecord(typeName: string, config: BaseDeviceConfig): DeviceRecord {
  return {
    record: { id: 1, typeName, configRevision: 1 },
    config,
    runtime: { status: 'ok', lifecycleStatus: 'ready', effectiveStatus: 'ok' },
  }
}

test('gpio-switch: single changed field produces a config diff with only that key', () => {
  const device = new GpioSwitchDevice()
  const current = GpioSwitchDevice.defaultConfig()
  const record = makeRecord(device.typeName, current)
  const draft: GpioSwitchConfigDraft & { typeName: string } = { ...current, inverted: !current.inverted, typeName: device.typeName }

  const commands = device.buildEditCommands(record, draft)
  const updateConfig = commands.find(c => c.command === 'updateConfig')

  assert.ok(updateConfig, 'expected an updateConfig command')
  assert.deepEqual(Object.keys(updateConfig!.config ?? {}), ['inverted'])
  assert.equal(updateConfig!.config?.inverted, !current.inverted)
})

test('gpio-switch: name-only change goes through updateConfig, not a separate rename command', () => {
  const device = new GpioSwitchDevice()
  const current = GpioSwitchDevice.defaultConfig()
  const record = makeRecord(device.typeName, current)
  const draft: GpioSwitchConfigDraft & { typeName: string } = { ...current, name: 'Renamed switch', typeName: device.typeName }

  const commands = device.buildEditCommands(record, draft)

  assert.equal(commands.find(c => c.command === 'rename'), undefined)
  const updateConfig = commands.find(c => c.command === 'updateConfig')
  assert.ok(updateConfig, 'expected an updateConfig command')
  assert.deepEqual(Object.keys(updateConfig!.config ?? {}), ['name'])
  assert.equal(updateConfig!.config?.name, 'Renamed switch')
})

test('gpio-switch: enabled-only change goes through updateConfig, not separate enable/disable commands', () => {
  const device = new GpioSwitchDevice()
  const current = GpioSwitchDevice.defaultConfig()
  const record = makeRecord(device.typeName, current)
  const draft: GpioSwitchConfigDraft & { typeName: string } = { ...current, enabled: !current.enabled, typeName: device.typeName }

  const commands = device.buildEditCommands(record, draft)

  assert.equal(commands.find(c => c.command === 'enable' || c.command === 'disable'), undefined)
  const updateConfig = commands.find(c => c.command === 'updateConfig')
  assert.ok(updateConfig, 'expected an updateConfig command')
  assert.deepEqual(Object.keys(updateConfig!.config ?? {}), ['enabled'])
  assert.equal(updateConfig!.config?.enabled, !current.enabled)
})

test('gpio-switch: no changes produces no updateConfig command', () => {
  const device = new GpioSwitchDevice()
  const current = GpioSwitchDevice.defaultConfig()
  const record = makeRecord(device.typeName, current)
  const draft: GpioSwitchConfigDraft & { typeName: string } = { ...current, typeName: device.typeName }

  const commands = device.buildEditCommands(record, draft)

  assert.equal(commands.find(c => c.command === 'updateConfig'), undefined)
})

test('ssd1306: nested layout change diffs only the layout key', () => {
  const device = new Ssd1306Device()
  const current = Ssd1306Device.defaultConfig()
  const record = makeRecord(device.typeName, current)
  const draft: Ssd1306ConfigDraft & { typeName: string } = {
    ...current,
    layout: {
      ...current.layout,
      pages: current.layout.pages.map(page => ({ ...page, name: `${page.name} renamed` })),
    },
    typeName: device.typeName,
  }

  const commands = device.buildEditCommands(record, draft)
  const updateConfig = commands.find(c => c.command === 'updateConfig')

  assert.ok(updateConfig, 'expected an updateConfig command')
  assert.deepEqual(Object.keys(updateConfig!.config ?? {}), ['layout'])
})

test('ssd1306: bus reassignment emits setDeps and never leaks the bus id into config', () => {
  const device = new Ssd1306Device()
  const current = Ssd1306Device.defaultConfig()
  const record = makeRecord(device.typeName, current)
  const draft: Ssd1306ConfigDraft & { typeName: string } = {
    ...current,
    i2cBusDeviceId: current.i2cBusDeviceId + 1,
    typeName: device.typeName,
  }

  const commands = device.buildEditCommands(record, draft)
  const setDeps = commands.find(c => c.command === 'setDeps')
  const updateConfig = commands.find(c => c.command === 'updateConfig')

  assert.ok(setDeps, 'expected a setDeps command')
  assert.equal(setDeps!.deps?.[0]?.deviceId, draft.i2cBusDeviceId)
  if (updateConfig) {
    assert.equal('i2cBusDeviceId' in (updateConfig.config ?? {}), false)
  }
})

test('thermostat: dependency-id-only change leaves the config diff empty but updates deps', () => {
  const device = new ThermostatDevice()
  const current: ThermostatConfigDraft = {
    ...ThermostatDevice.defaultConfig(),
    temperatureSensorDeviceId: 10,
    switchDeviceId: 20,
  }
  const record = makeRecord(device.typeName, current)
  const draft: ThermostatConfigDraft & { typeName: string } = {
    ...current,
    temperatureSensorDeviceId: 99,
    typeName: device.typeName,
  }

  const commands = device.buildEditCommands(record, draft)
  const updateConfig = commands.find(c => c.command === 'updateConfig')

  assert.ok(updateConfig, 'expected an updateConfig command carrying the deps snapshot')
  assert.deepEqual(updateConfig!.config, {})
  assert.ok(updateConfig!.deps?.some((dep: { role: string; deviceId: number }) => dep.role === 'temperature_sensor' && dep.deviceId === 99))
})

test('thermostat: ms fields whose rounded encoding is unchanged are not reported as diffs', () => {
  const device = new ThermostatDevice()
  const current: ThermostatConfigDraft = {
    ...ThermostatDevice.defaultConfig(),
    checkIntervalMs: 1000.2,
  }
  const record = makeRecord(device.typeName, current)
  const draft: ThermostatConfigDraft & { typeName: string } = {
    ...current,
    checkIntervalMs: 1000.4,
    typeName: device.typeName,
  }

  const commands = device.buildEditCommands(record, draft)

  assert.equal(commands.find(c => c.command === 'updateConfig'), undefined)
})

test('ds18b20: dependency-id-only change leaves the config diff empty but updates deps', () => {
  const device = new Ds18b20Device()
  const current: Ds18b20ConfigDraft = {
    ...Ds18b20Device.defaultConfig(),
    dependencyDeviceId: 10,
    address: '28FF641D621603AD',
  }
  const record = makeRecord(device.typeName, current)
  const draft: Ds18b20ConfigDraft & { typeName: string } = {
    ...current,
    dependencyDeviceId: 42,
    typeName: device.typeName,
  }

  const commands = device.buildEditCommands(record, draft)
  const updateConfig = commands.find(c => c.command === 'updateConfig')

  assert.ok(updateConfig, 'expected an updateConfig command carrying the deps snapshot')
  assert.deepEqual(updateConfig!.config, {})
  assert.ok(updateConfig!.deps?.some((dep: { role: string; deviceId: number }) => dep.role === 'onewire_bus' && dep.deviceId === 42))
})

test('gpio-switch: create payload never leaks the create-draft typeName into config', () => {
  const device = new GpioSwitchDevice()
  const draft = device.createDefaultCreateDraft()

  const payload = device.buildCreatePayload(draft)

  assert.equal(payload.typeName, device.typeName)
  assert.equal('typeName' in payload.config, false)
})

test('thermostat: create payload never leaks the create-draft typeName into config', () => {
  const device = new ThermostatDevice()
  const draft = device.createDefaultCreateDraft()

  const payload = device.buildCreatePayload(draft)

  assert.equal(payload.typeName, device.typeName)
  assert.equal('typeName' in payload.config, false)
})
