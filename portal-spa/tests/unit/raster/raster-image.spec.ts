import assert from 'node:assert/strict'
import test from 'node:test'

import { Gray8RasterImageCodec, Mono1RasterImageCodec, RasterImagePayload, Rgb565RasterImageCodec } from '../../../src/raster/raster-image.ts'

test('raster payload class validates dimensions and encoding', () => {
  const payload = new RasterImagePayload('mono1', 8, 2, Uint8Array.from([0xff, 0x00]))
  assert.equal(payload.width, 8)
  assert.equal(payload.height, 2)
  assert.equal(payload.byteLength, 2)
  assert.equal(globalThis.atob(payload.toBase64()).length, 2)

  assert.throws(() => RasterImagePayload.fromBase64('mono1', 8, 2, 'AA'))
})

test('mono codec round-trips packed raster bytes', () => {
  const codec = new Mono1RasterImageCodec()
  const bytes = Uint8Array.from([0b10000001, 0b01000000])
  const encoded = codec.encode(bytes, 8, 2)
  assert.deepEqual(Array.from(codec.decode(encoded, 8, 2)), Array.from(bytes))
  assert.equal(codec.placeholder(8, 2).byteLength, 2)
})

test('gray8 codec uses one byte per pixel', () => {
  const codec = new Gray8RasterImageCodec()
  const bytes = Uint8Array.from([0, 32, 64, 255])
  const encoded = codec.encode(bytes, 2, 2)
  assert.deepEqual(Array.from(codec.decode(encoded, 2, 2)), Array.from(bytes))
  assert.equal(codec.placeholder(2, 2).byteLength, 4)
})

test('rgb565 codec uses two bytes per pixel', () => {
  const codec = new Rgb565RasterImageCodec()
  const bytes = Uint8Array.from([0x00, 0x00, 0xff, 0xff, 0x12, 0x34, 0xab, 0xcd])
  const encoded = codec.encode(bytes, 2, 2)
  assert.deepEqual(Array.from(codec.decode(encoded, 2, 2)), Array.from(bytes))
  assert.equal(codec.placeholder(2, 2).byteLength, 8)
})
