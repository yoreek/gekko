import assert from 'node:assert/strict'
import test from 'node:test'

import { SPI_BUS_DEVICE_TYPE_ID, ST7735_DEVICE_TYPE_ID, deviceTypeIdFromName, deviceTypeName } from '../../../../src/models/device-type-ids.ts'
import { SpiBusDevice, defaultSpiBusConfig } from '../../../../src/models/devices/spi-bus.ts'
import { Device as St7735Device } from '../../../../src/models/devices/st7735/device.ts'

test('resolves spi bus and st7735 device type ids', () => {
  assert.equal(deviceTypeIdFromName('spi_bus'), SPI_BUS_DEVICE_TYPE_ID)
  assert.equal(deviceTypeIdFromName('st7735'), ST7735_DEVICE_TYPE_ID)
  assert.equal(deviceTypeName(SPI_BUS_DEVICE_TYPE_ID), 'spi_bus')
  assert.equal(deviceTypeName(ST7735_DEVICE_TYPE_ID), 'st7735')
})

test('normalizes spi bus defaults and st7735 dependency fields', () => {
  const spiBus = new SpiBusDevice().normalizeConfig({})
  const st7735 = new St7735Device().normalizeConfig({
    spiBusDeviceId: 12,
    chipSelectPin: 5,
    layout: {
      schemaVersion: 1,
      activePageId: 'main',
      pages: [],
    },
  })

  assert.deepEqual(spiBus, defaultSpiBusConfig())
  assert.equal(spiBus.sckPin, 18)
  assert.equal(spiBus.mosiPin, 23)
  assert.equal(spiBus.misoPin, -1)
  assert.equal(st7735.spiBusDeviceId, 12)
  assert.equal(st7735.chipSelectPin, 5)
  assert.equal(st7735.layout.colorMode, 'rgb565')
})
