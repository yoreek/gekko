import assert from 'node:assert/strict'
import test from 'node:test'

import { rasterImagePixelConverters, rasterImageRenderAdapters } from '../../../src/raster/raster-image-format-registry.ts'

test('registry exposes pixel converters for all raster formats', () => {
  assert.equal(rasterImagePixelConverters.mono1.bytesPerPixel, 1 / 8)
  assert.equal(rasterImagePixelConverters.gray8.bytesPerPixel, 1)
  assert.equal(rasterImagePixelConverters.rgb565.bytesPerPixel, 2)

  assert.deepEqual(rasterImagePixelConverters.mono1.convert(0, 0, 0, 0, 128), [0])
  assert.deepEqual(rasterImagePixelConverters.mono1.convert(255, 255, 255, 255, 128), [1])
  assert.deepEqual(rasterImagePixelConverters.gray8.convert(255, 255, 255, 255, 128), [255])
  assert.equal(rasterImagePixelConverters.rgb565.convert(255, 0, 0, 255, 128).length, 2)
})

test('registry exposes render adapters with consistent byte counts', () => {
  assert.equal(rasterImageRenderAdapters.mono1.expectedBytes(8, 2), 2)
  assert.equal(rasterImageRenderAdapters.gray8.expectedBytes(2, 2), 4)
  assert.equal(rasterImageRenderAdapters.rgb565.expectedBytes(2, 2), 8)
})
