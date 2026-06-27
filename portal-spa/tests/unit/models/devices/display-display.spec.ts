import assert from 'node:assert/strict'
import test from 'node:test'

import { ssd1306Display, st7735Display } from '../../../../src/models/devices/display/display.ts'

test('display factories bind fixed bitmap formats', () => {
  assert.equal(ssd1306Display.supportsBitmapFormat('mono1'), true)
  assert.equal(ssd1306Display.supportsBitmapFormat('rgb565'), false)
  assert.equal(st7735Display.supportsBitmapFormat('rgb565'), true)
  assert.equal(st7735Display.supportsBitmapFormat('mono1'), false)
})

test('bitmap widgets come from the concrete display factory', () => {
  const oledBitmap = ssd1306Display.createBitmapPlaceholder(12, 8, 2)
  const tftBitmap = st7735Display.createWidget('bitmap', 3)

  assert.equal(oledBitmap.bitmapFormat, 'mono1')
  assert.equal(oledBitmap.id, 'bitmap-2')
  assert.equal(tftBitmap.type, 'bitmap')
  assert.equal(tftBitmap.bitmapFormat, 'rgb565')
  assert.equal(tftBitmap.id, 'bitmap-3')
})
