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
})
