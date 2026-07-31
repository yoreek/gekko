import assert from 'node:assert/strict'
import test from 'node:test'

import { SPI_BUS_DEVICE_TYPE_ID, ST7735_DEVICE_TYPE_ID, deviceTypeIdFromName, deviceTypeName } from '../../../../src/models/device-type-ids.ts'
import { SpiBusDevice } from '../../../../src/models/devices/spi-bus.ts'
import { Ssd1306Device } from '../../../../src/models/devices/ssd1306/device.ts'
import { St7735Device } from '../../../../src/models/devices/st7735/device.ts'
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
    resetPin: 255,
    rotation: 1,
    layout: {
      schemaVersion: 1,
      activePageId: 'main',
      pages: [],
    },
  })

  assert.deepEqual(spiBus, SpiBusDevice.defaultConfig())
  assert.equal(spiBus.sckPin, 18)
  assert.equal(spiBus.mosiPin, 23)
  assert.equal(spiBus.misoPin, 255)
  assert.equal(st7735.spiBusDeviceId, 12)
  assert.equal(st7735.chipSelectPin, 5)
  assert.equal(st7735.dcPin, 4)
  assert.equal(st7735.resetPin, 255)
  assert.equal(st7735.rotation, 1)
  assert.equal(st7735.panel, 'black18')
  assert.equal(st7735.layout.colorMode, 'rgb565')

  const encoded = St7735Device.encodeConfig(st7735)
  assert.equal(encoded.dcPin, 4)
  assert.equal(encoded.resetPin, 255)
  assert.equal(encoded.rotation, 1)
  assert.equal(encoded.panel, 'black18')
  assert.equal(encoded.width, 128)
  assert.equal(encoded.height, 160)
})

test('st7735 rotation is no longer truncated to portrait/landscape, and width/height always follow panel', () => {
  const rotated180 = new St7735Device().normalizeConfig({ spiBusDeviceId: 12, rotation: 2 })
  assert.equal(rotated180.rotation, 2)
  const rotated270 = new St7735Device().normalizeConfig({ spiBusDeviceId: 12, rotation: 3 })
  assert.equal(rotated270.rotation, 3)

  const mini = new St7735Device().normalizeConfig({ spiBusDeviceId: 12, panel: 'mini096', width: 999, height: 999 })
  assert.equal(mini.panel, 'mini096')
  assert.equal(mini.width, 80)
  assert.equal(mini.height, 160)

  const unknownPanel = new St7735Device().normalizeConfig({ spiBusDeviceId: 12, panel: 'not-a-real-panel' })
  assert.equal(unknownPanel.panel, 'black18')
  assert.equal(unknownPanel.width, 128)
  assert.equal(unknownPanel.height, 160)
})

test('effective display size swaps axes for landscape rotations', () => {
  assert.deepEqual(resolveDisplayEffectiveSize(128, 64, 0), { effectiveWidth: 128, effectiveHeight: 64 })
  assert.deepEqual(resolveDisplayEffectiveSize(128, 64, 1), { effectiveWidth: 64, effectiveHeight: 128 })
})

test('ssd1306 defaults keep the mounted orientation stable', () => {
  const ssd1306 = new Ssd1306Device().normalizeConfig(
    {
      i2cAddress: 0x3C,
      layout: {
        schemaVersion: 1,
        activePageId: 'main',
        pages: [],
      },
    },
    [{ role: 'i2c_bus', deviceId: 12 }],
  )

  assert.equal(ssd1306.rotation, 0)
  assert.equal(ssd1306.panel, '128x64')
  assert.equal(ssd1306.dependencyDeviceId, 12)
  assert.equal(Ssd1306Device.encodeConfig(ssd1306).rotation, 0)
  assert.equal(Ssd1306Device.encodeConfig(ssd1306).panel, '128x64')
  assert.equal('i2cBusDeviceId' in Ssd1306Device.encodeConfig(ssd1306), false)
})

test('ssd1306 rotation is no longer truncated, and panel selects/derives geometry', () => {
  const rotated180 = new Ssd1306Device().normalizeConfig({ rotation: 2 }, [{ role: 'i2c_bus', deviceId: 12 }])
  assert.equal(rotated180.rotation, 2)
  const rotated270 = new Ssd1306Device().normalizeConfig({ rotation: 3 }, [{ role: 'i2c_bus', deviceId: 12 }])
  assert.equal(rotated270.rotation, 3)

  const preset = new Ssd1306Device().normalizeConfig({ panel: '96x16', width: 1, height: 1 }, [{ role: 'i2c_bus', deviceId: 12 }])
  assert.equal(preset.panel, '96x16')
  assert.equal(preset.width, 96)
  assert.equal(preset.height, 16)

  // No panel supplied but width/height match a preset exactly -> inferred, not left as raw numbers.
  const inferred = new Ssd1306Device().normalizeConfig({ width: 128, height: 32 }, [{ role: 'i2c_bus', deviceId: 12 }])
  assert.equal(inferred.panel, '128x32')

  // Arbitrary dimensions with no matching preset -> custom, values preserved as-is.
  const custom = new Ssd1306Device().normalizeConfig({ width: 70, height: 45 }, [{ role: 'i2c_bus', deviceId: 12 }])
  assert.equal(custom.panel, 'custom')
  assert.equal(custom.width, 70)
  assert.equal(custom.height, 45)

  const encoded = Ssd1306Device.encodeConfig(custom)
  assert.equal(encoded.panel, 'custom')
  assert.equal(encoded.width, 70)
  assert.equal(encoded.height, 45)
})
