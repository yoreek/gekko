import assert from 'node:assert/strict'
import test from 'node:test'

import { useDisplayBitmapRenderLock } from '../../../../../src/composables/display/useDisplayBitmapRenderLock.ts'

test('freezes bitmap render only while resizing the active bitmap widget', () => {
  const lock = useDisplayBitmapRenderLock()

  lock.setBitmapRenderLock('a', 'a', 'resize', true)
  assert.equal(lock.bitmapRenderFrozen.value, true)

  lock.setBitmapRenderLock('a', 'a', 'drag', true)
  assert.equal(lock.bitmapRenderFrozen.value, false)

  lock.setBitmapRenderLock('a', 'b', 'resize', true)
  assert.equal(lock.bitmapRenderFrozen.value, false)

  lock.setBitmapRenderLock('a', 'a', 'resize', false)
  assert.equal(lock.bitmapRenderFrozen.value, false)
})
