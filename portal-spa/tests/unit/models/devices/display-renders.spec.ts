import assert from 'node:assert/strict'
import test from 'node:test'

import { Gray8Render, Mono1Render, Rgb565Render } from '../../../../src/models/devices/display/renders/index.ts'

type ImageDataMock = { data: Uint8ClampedArray }

function createCanvas() {
  let lastImageData: ImageDataMock | null = null
  const context = {
    createImageData: (width: number, height: number) => ({ data: new Uint8ClampedArray(width * height * 4) }),
    putImageData: (image: ImageDataMock) => {
      lastImageData = image
    },
  } as CanvasRenderingContext2D
  return {
    width: 0,
    height: 0,
    getContext: () => context,
    get lastImageData() {
      return lastImageData
    },
  } as unknown as HTMLCanvasElement & { readonly lastImageData: ImageDataMock | null }
}

function toBase64(bytes: number[]): string {
  return Buffer.from(bytes).toString('base64')
}

test('mono1 render normalizes canvas and draws pixels', () => {
  const render = new Mono1Render()
  const canvas = createCanvas()

  render.draw(canvas, toBase64([0b10000001]), 0, 0)

  assert.equal(canvas.width, 1)
  assert.equal(canvas.height, 1)
  assert.equal(canvas.lastImageData?.data[0], 255)
  assert.equal(canvas.lastImageData?.data[1], 255)
  assert.equal(canvas.lastImageData?.data[2], 255)
  assert.equal(canvas.lastImageData?.data[3], 255)
})

test('gray8 render draws grayscale pixels', () => {
  const render = new Gray8Render()
  const canvas = createCanvas()

  render.draw(canvas, toBase64([0, 128, 255]), 3, 1)

  assert.equal(canvas.width, 3)
  assert.equal(canvas.height, 1)
  assert.equal(canvas.lastImageData?.data[0], 0)
  assert.equal(canvas.lastImageData?.data[4], 128)
  assert.equal(canvas.lastImageData?.data[8], 255)
})

test('rgb565 render draws rgb pixels', () => {
  const render = new Rgb565Render()
  const canvas = createCanvas()

  render.draw(canvas, toBase64([0xF8, 0x00]), 1, 1)

  assert.equal(canvas.width, 1)
  assert.equal(canvas.height, 1)
  assert.equal(canvas.lastImageData?.data[0], 255)
  assert.equal(canvas.lastImageData?.data[1], 0)
  assert.equal(canvas.lastImageData?.data[2], 0)
  assert.equal(canvas.lastImageData?.data[3], 255)
})
