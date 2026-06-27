import assert from 'node:assert/strict'
import test from 'node:test'

import { useDisplayBitmapImportState } from '../../../../../src/composables/display/useDisplayBitmapImportState.ts'

test('queues st7735 bitmap import and emits image data', async () => {
  const file = new File([new Uint8Array([1, 2, 3])], 'bitmap.png', { type: 'image/png' })
  const emitted: Array<{ bitmapData: string }> = []
  const state = useDisplayBitmapImportState(
    () => ({ type: 'bitmap', width: 8, height: 8 } as const),
    patch => emitted.push(patch),
    key => key,
    async (_file, width, height) => ({ imageData: globalThis.btoa(`rgb565:${width}x${height}`) }),
    (width, height) => globalThis.btoa(`placeholder:${width}x${height}`),
    () => true,
  )

  await state.queueBitmapImport(file, 16, 8)

  assert.equal(emitted.length, 1)
  assert.equal(typeof emitted[0]?.bitmapData, 'string')
})
