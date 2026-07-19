import assert from 'node:assert/strict'
import test from 'node:test'

import { ssd1306Display, st7735Display } from '../../../../src/models/devices/display/display.ts'
import {
  cloneDisplayBitmapWidget,
  resolveDisplayBitmapDimensionUpdate,
} from '../../../../src/models/devices/display/widgets/index.ts'

test('display factories bind fixed bitmap formats', () => {
  assert.equal(ssd1306Display.supportsColor, false)
  assert.equal(st7735Display.supportsColor, true)
  assert.equal(ssd1306Display.supportsBitmapFormat('mono1'), true)
  assert.equal(ssd1306Display.supportsBitmapFormat('rgb565'), false)
  assert.equal(st7735Display.supportsBitmapFormat('rgb565'), true)
  assert.equal(st7735Display.supportsBitmapFormat('mono1'), false)
})

test('RGB565 display enforces the firmware bitmap payload limit', () => {
  assert.equal(st7735Display.maxBitmapBytes, 3072)
  assert.equal(st7735Display.supportsBitmapSize(32, 48), true)
  assert.equal(st7735Display.supportsBitmapSize(33, 48), false)

  const source = st7735Display.createBitmapPlaceholder(16, 16)
  const rejected = st7735Display.resizeWidget(source, { width: 160, height: 128 })
  assert.equal(rejected, source)
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

test('bitmap data is rescaled when the widget width changes', () => {
  const source = ssd1306Display.createBitmapPlaceholder(4, 4)
  const resized = ssd1306Display.resizeWidget(source, { width: 8, height: 8 })

  assert.equal(resized.width, 8)
  assert.equal(resized.height, 8)
  assert.equal(typeof resized.bitmapData, 'string')
  assert.notEqual(resized.bitmapData, source.bitmapData)
  assert.equal(globalThis.atob(resized.bitmapData).length, 8)
})

test('bitmap dimension helper preserves aspect ratio when requested', () => {
  const source = {
    ...ssd1306Display.createBitmapPlaceholder(16, 8),
    keepAspectRatio: true,
  }

  assert.deepEqual(resolveDisplayBitmapDimensionUpdate(source, 'width', 32), { width: 32, height: 16 })
  assert.deepEqual(resolveDisplayBitmapDimensionUpdate(source, 'height', 4), { width: 8, height: 4 })
  assert.equal(resolveDisplayBitmapDimensionUpdate(source, 'width', 'invalid'), null)
})

test('bitmap clone copies style flags without changing the widget shape', () => {
  const source = {
    ...st7735Display.createBitmapPlaceholder(16, 16),
    styleFlags: { filled: true, inverted: false, wrap: true },
  }
  const cloned = cloneDisplayBitmapWidget(source)

  assert.deepEqual(cloned, source)
  assert.notEqual(cloned, source)
  assert.notEqual(cloned.styleFlags, source.styleFlags)
})
