import assert from 'node:assert/strict'
import test from 'node:test'

import { createDebouncedCallback } from '../../../src/utils/debounced-callback.ts'

function wait(delayMs: number): Promise<void> {
  return new Promise(resolve => setTimeout(resolve, delayMs))
}

test('debounced callback emits only the latest scheduled value', async () => {
  const values: number[] = []
  const debounced = createDebouncedCallback<number>(value => values.push(value), 15)

  debounced.schedule(10)
  debounced.schedule(20)
  debounced.schedule(30)

  await wait(30)

  assert.deepEqual(values, [30])
})

test('debounced callback can cancel a pending value', async () => {
  const values: number[] = []
  const debounced = createDebouncedCallback<number>(value => values.push(value), 15)

  debounced.schedule(42)
  debounced.cancel()

  await wait(30)

  assert.deepEqual(values, [])
})
