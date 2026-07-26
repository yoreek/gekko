import assert from 'node:assert/strict'
import test from 'node:test'

import { createSeedMockDatabase } from '../../../src/mock/database.ts'

test('seed bitmap payloads match their declared dimensions', () => {
  const database = createSeedMockDatabase()
  const oled = database.devices.find(device => device.record.typeName === 'ssd1306')
  const tft = database.devices.find(device => device.record.typeName === 'st7735')
  const oledBitmap = oled?.config.layout.pages[0]?.widgets.find(widget => widget.type === 'bitmap')
  const tftBitmap = tft?.config.layout.pages[0]?.widgets.find(widget => widget.type === 'bitmap')

  assert.ok(oledBitmap && oledBitmap.type === 'bitmap')
  assert.ok(tftBitmap && tftBitmap.type === 'bitmap')
  assert.equal(Buffer.from(oledBitmap.bitmapData, 'base64').length, Math.ceil(oledBitmap.width / 8) * oledBitmap.height)
  assert.equal(Buffer.from(tftBitmap.bitmapData, 'base64').length, tftBitmap.width * tftBitmap.height * 2)
  assert.equal(oledBitmap.keepAspectRatio, true)
  assert.equal(tftBitmap.keepAspectRatio, true)

  const htu21 = database.devices.find(device => device.record.typeName === 'htu21')
  assert.equal(htu21?.record.configRevision, 1)
  assert.equal(htu21?.config.i2cAddress, 0x40)

  const aht10 = database.devices.find(device => device.record.typeName === 'aht10')
  assert.equal(aht10?.record.configRevision, 1)
  assert.equal(aht10?.config.i2cAddress, 0x38)

  const analogOutput = database.devices.find(device => device.record.typeName === 'analog_output')
  assert.ok(analogOutput)
  assert.equal(analogOutput.record.configRevision, 2)
  assert.equal(analogOutput.config.startupState, 35)
  assert.equal(analogOutput.config.safeState, 0)
  assert.equal(analogOutput.runtime.output.state, 35)
  assert.equal('channels' in analogOutput.config, false)

  const composer = database.devices.find(device => device.record.typeName === 'analog_output_composer')
  assert.ok(composer)
  const composerWidget = database.dashboardLayout.panels
    .flatMap(panel => panel.widgets)
    .find(widget => widget[0] === composer.record.id)
  assert.ok(composerWidget)
  assert.equal(composerWidget[3], 1)
})
