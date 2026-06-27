import assert from 'node:assert/strict'
import test from 'node:test'

import { RasterImageRenderer } from '../../../src/raster/raster-image-renderer.ts'

function createCanvas() {
  const context = {
    createImageData: (width: number, height: number) => ({ data: new Uint8ClampedArray(width * height * 4) }),
    fillRect: () => {},
    clearRect: () => {},
    putImageData: () => {},
    getImageData: () => ({ data: new Uint8ClampedArray(0) }),
  } as CanvasRenderingContext2D
  return {
    width: 0,
    height: 0,
    getContext: () => context,
  } as unknown as HTMLCanvasElement
}

test('renderer normalizes canvas dimensions before drawing', () => {
  const renderer = new RasterImageRenderer()
  const canvas = createCanvas()

  renderer.drawMonochromeCanvas(canvas, '', 0, 0, false)

  assert.equal(canvas.width, 1)
  assert.equal(canvas.height, 1)
})
