import assert from 'node:assert/strict'
import test from 'node:test'

import { useRasterImageImportState } from '../../../../src/composables/display/useRasterImageImportState.ts'

test('queues raster image import and emits image data', async () => {
  const file = new File([new Uint8Array([1, 2, 3])], 'bitmap.png', { type: 'image/png' })
  const emitted: Array<{ bitmapData: string }> = []
  const state = useRasterImageImportState(
    () => ({ type: 'bitmap', width: 8, height: 8 }),
    patch => emitted.push(patch),
    key => key,
    async (_file, width, height, threshold) => ({
      format: 'mono1',
      width,
      height,
      imageData: `data:${width}x${height}:${threshold}`,
      byteLength: 8,
    }),
    (width, height) => `placeholder:${width}x${height}`,
    () => true,
  )

  await state.queueImageImport(file, 16, 8)

  assert.equal(emitted.length, 1)
  assert.deepEqual(emitted[0], { bitmapData: 'data:16x8:128' })
  assert.equal(state.imagePreviewFrozen.value, false)
  assert.equal(state.imageError.value, '')
})

test('clearImage emits a placeholder payload and resets the transient file', () => {
  const emitted: Array<{ bitmapData: string }> = []
  const state = useRasterImageImportState(
    () => ({ type: 'bitmap', width: 8, height: 8 }),
    patch => emitted.push(patch),
    key => key,
    async () => ({ format: 'mono1', width: 8, height: 8, imageData: 'data', byteLength: 8 }),
    (width, height) => `placeholder:${width}x${height}`,
    () => true,
  )

  state.clearImage()

  assert.deepEqual(emitted, [{ bitmapData: 'placeholder:1x1' }])
  assert.equal(state.importedImageFile.value, null)
  assert.equal(state.imageError.value, '')
})

test('ignores imports when the widget is not raster-capable', async () => {
  const emitted: Array<{ bitmapData: string }> = []
  const state = useRasterImageImportState(
    () => ({ type: 'text', width: 8, height: 8 }),
    patch => emitted.push(patch),
    key => key,
    async () => ({ format: 'mono1', width: 8, height: 8, imageData: 'data', byteLength: 8 }),
    (width, height) => `placeholder:${width}x${height}`,
    () => false,
  )

  await state.queueImageImport(new File([new Uint8Array([1])], 'bitmap.png'), 8, 8)

  assert.deepEqual(emitted, [])
})
