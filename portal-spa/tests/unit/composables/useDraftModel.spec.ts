import assert from 'node:assert/strict'
import test from 'node:test'

import { useDraftModel } from '../../../src/composables/useDraftModel.ts'

interface Draft {
  name: string
  enabled: boolean
}

test('useDraftModel: update emits a shallow copy with one field replaced', () => {
  const props = { modelValue: { name: 'a', enabled: true } as Draft }
  const emitted: Draft[] = []
  const { update } = useDraftModel(props, (_event, value) => {
    emitted.push(value)
  })

  update('name', 'b')

  assert.equal(emitted.length, 1)
  assert.deepEqual(emitted[0], { name: 'b', enabled: true })
  assert.notEqual(emitted[0], props.modelValue)
  assert.equal(props.modelValue.name, 'a')
})

test('useDraftModel: update reads the current modelValue at call time', () => {
  const props = { modelValue: { name: 'a', enabled: true } as Draft }
  const emitted: Draft[] = []
  const { update } = useDraftModel(props, (_event, value) => {
    emitted.push(value)
  })

  props.modelValue = { name: 'renamed', enabled: false }
  update('enabled', true)

  assert.deepEqual(emitted[0], { name: 'renamed', enabled: true })
})
