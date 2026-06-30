import assert from 'node:assert/strict'
import test from 'node:test'

import { SPI_BUS_DEVICE_TYPE_ID, ST7735_DEVICE_TYPE_ID, deviceTypeIdFromName, deviceTypeName } from '../../../../src/models/device-type-ids.ts'
import { SpiBusDevice, defaultSpiBusConfig } from '../../../../src/models/devices/spi-bus.ts'
import { Device as Ssd1306Device } from '../../../../src/models/devices/ssd1306/device.ts'
import { Device as St7735Device } from '../../../../src/models/devices/st7735/device.ts'
import { resolveDisplayEffectiveSize } from '../../../../src/models/devices/display/orientation.ts'

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
    dcPin: 4,
    resetPin: -1,
    rotation: 1,
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
  assert.equal(st7735.dcPin, 4)
  assert.equal(st7735.resetPin, -1)
  assert.equal(st7735.rotation, 1)
  assert.equal(st7735.layout.colorMode, 'rgb565')

  const encoded = new St7735Device().encodeConfig(st7735)
  assert.equal(encoded.dcPin, 4)
  assert.equal(encoded.resetPin, -1)
  assert.equal(encoded.rotation, 1)
  assert.equal(encoded.width, 128)
  assert.equal(encoded.height, 160)
})

test('effective display size swaps axes for landscape rotations', () => {
  assert.deepEqual(resolveDisplayEffectiveSize(128, 64, 0), { effectiveWidth: 128, effectiveHeight: 64 })
  assert.deepEqual(resolveDisplayEffectiveSize(128, 64, 1), { effectiveWidth: 64, effectiveHeight: 128 })
})

test('ssd1306 defaults keep the mounted orientation stable', () => {
  const ssd1306 = new Ssd1306Device().normalizeConfig({
    i2cBusDeviceId: 12,
    i2cAddress: 0x3C,
    layout: {
      schemaVersion: 1,
      activePageId: 'main',
      pages: [],
    },
  })

  assert.equal(ssd1306.rotation, 0)
  assert.equal(new Ssd1306Device().encodeConfig(ssd1306).rotation, 0)
})
