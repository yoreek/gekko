import assert from 'node:assert/strict'
import test from 'node:test'

import { Gray8RasterImageImporter } from '../../../src/models/devices/display/widgets/gray8-bitmap-importer.ts'
import { Gray8RasterImagePayload } from '../../../src/raster/gray8/Gray8RasterImagePayload.ts'
import { Mono1RasterImageImporter } from '../../../src/models/devices/display/widgets/mono1-bitmap-importer.ts'
import { Mono1RasterImagePayload } from '../../../src/raster/mono1/Mono1RasterImagePayload.ts'
import { Rgb565RasterImageImporter } from '../../../src/models/devices/display/widgets/rgb565-bitmap-importer.ts'
import { Rgb565RasterImagePayload } from '../../../src/raster/rgb565/Rgb565RasterImagePayload.ts'

function createCanvas(width: number, height: number) {
  const pixels = new Uint8ClampedArray(width * height * 4)
  const calls: string[] = []
  const context = {
    imageSmoothingEnabled: true,
    fillStyle: '#000000',
    clearRect: () => {
      calls.push('clearRect')
    },
    fillRect: () => {
      calls.push('fillRect')
      for (let index = 0; index < pixels.length; index += 4) {
        pixels[index] = 255
        pixels[index + 1] = 255
        pixels[index + 2] = 255
        pixels[index + 3] = 255
      }
    },
    drawImage: () => {
      calls.push('drawImage')
      pixels[0] = 0
      pixels[1] = 0
      pixels[2] = 0
      pixels[3] = 255
    },
    getImageData: () => ({ data: pixels }),
  } as CanvasRenderingContext2D
  return {
    width,
    height,
    getContext: () => context,
    context,
    calls,
  } as unknown as HTMLCanvasElement
}

async function withBitmapEnvironment<T>(canvas: HTMLCanvasElement, run: () => Promise<T>): Promise<T> {
  const createImageBitmap = globalThis.createImageBitmap
  const documentBefore = globalThis.document
  const createElement = (documentBefore?.createElement ?? (() => { throw new Error('unexpected element request') })).bind(documentBefore)

  globalThis.createImageBitmap = async () => ({ width: canvas.width, height: canvas.height } as ImageBitmap)
  globalThis.document = {
    createElement: (tagName: string) => (tagName === 'canvas' ? canvas : createElement(tagName)),
  } as typeof document

  try {
    return await run()
  } finally {
    globalThis.createImageBitmap = createImageBitmap
    globalThis.document = documentBefore
  }
}

test('mono1 importer returns packed bitmap payload', async () => {
  const importer = new Mono1RasterImageImporter()
  const canvas = createCanvas(2, 2)
  const file = new File([new Uint8Array([1, 2, 3])], 'bitmap.png', { type: 'image/png' })

  const result = await withBitmapEnvironment(canvas, async () => await importer.importFromFile(file, 2, 2, 128))

  assert.equal(result.format, 'mono1')
  assert.equal(result.width, 2)
  assert.equal(result.height, 2)
  assert.equal(result.byteLength, Mono1RasterImagePayload.resolveByteLength(2, 2))
  assert.equal(result.imageData, Buffer.from([0b01000000, 0b11000000]).toString('base64'))
  assert.deepEqual((canvas as HTMLCanvasElement & { calls: string[] }).calls, ['clearRect', 'fillRect', 'drawImage'])
  assert.equal((canvas as HTMLCanvasElement & { context: CanvasRenderingContext2D }).context.fillStyle, '#ffffff')
})

test('gray8 importer returns grayscale payload', async () => {
  const importer = new Gray8RasterImageImporter()
  const canvas = createCanvas(2, 2)
  const file = new File([new Uint8Array([1, 2, 3])], 'bitmap.png', { type: 'image/png' })

  const result = await withBitmapEnvironment(canvas, async () => await importer.importFromFile(file, 2, 2, 128))

  assert.equal(result.format, 'gray8')
  assert.equal(result.width, 2)
  assert.equal(result.height, 2)
  assert.equal(result.byteLength, Gray8RasterImagePayload.resolveByteLength(2, 2))
})

test('rgb565 importer returns color payload', async () => {
  const importer = new Rgb565RasterImageImporter()
  const canvas = createCanvas(2, 2)
  const file = new File([new Uint8Array([1, 2, 3])], 'bitmap.png', { type: 'image/png' })

  const result = await withBitmapEnvironment(canvas, async () => await importer.importFromFile(file, 2, 2, 128))

  assert.equal(result.format, 'rgb565')
  assert.equal(result.width, 2)
  assert.equal(result.height, 2)
  assert.equal(result.byteLength, Rgb565RasterImagePayload.resolveByteLength(2, 2))
})
