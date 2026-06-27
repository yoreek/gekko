import assert from 'node:assert/strict'
import test from 'node:test'

import { RasterImageImporter } from '../../../src/raster/raster-image-importer.ts'
import { RasterImagePayload } from '../../../src/raster/raster-image.ts'

function createCanvas(width: number, height: number) {
  const pixels = new Uint8ClampedArray(width * height * 4)
  const context = {
    imageSmoothingEnabled: true,
    clearRect: () => {},
    drawImage: () => {},
    getImageData: () => ({ data: pixels }),
  } as CanvasRenderingContext2D
  return {
    width,
    height,
    getContext: () => context,
    context,
  } as unknown as HTMLCanvasElement
}

test('imports mono1 raster data into packed bytes', async () => {
  const importer = new RasterImageImporter()
  const file = new File([new Uint8Array([1, 2, 3])], 'bitmap.png', { type: 'image/png' })
  const canvas = createCanvas(2, 2)
  const createImageBitmap = globalThis.createImageBitmap
  const documentBefore = globalThis.document
  const createElement = (documentBefore?.createElement ?? (() => { throw new Error('unexpected element request') })).bind(documentBefore)

  globalThis.createImageBitmap = async () => ({ width: 2, height: 2 } as ImageBitmap)
  globalThis.document = {
    createElement: (tagName: string) => (tagName === 'canvas' ? canvas : createElement(tagName)),
  } as typeof document

  try {
    const result = await importer.importFromFile(file, 2, 2, { format: 'mono1', threshold: 128 })
    assert.equal(result.format, 'mono1')
    assert.equal(result.width, 2)
    assert.equal(result.height, 2)
    assert.equal(result.byteLength, RasterImagePayload.resolveByteLength('mono1', 2, 2))
  } finally {
    globalThis.createImageBitmap = createImageBitmap
    globalThis.document = documentBefore
  }
})
