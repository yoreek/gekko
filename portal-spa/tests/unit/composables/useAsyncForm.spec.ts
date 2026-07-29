import assert from 'node:assert/strict'
import test from 'node:test'

import { useAsyncForm } from '../../../src/composables/useAsyncForm.ts'

interface Source {
  id: number
  name: string
}

interface Draft {
  name: string
}

function deferred<T>() {
  let resolve!: (value: T) => void
  let reject!: (reason?: unknown) => void
  const promise = new Promise<T>((resolvePromise, rejectPromise) => {
    resolve = resolvePromise
    reject = rejectPromise
  })
  return { promise, resolve, reject }
}

function createForm(
  load: () => Promise<Source>,
  save: (source: Source, draft: Draft) => Promise<Source> = async (source, draft) => ({
    ...source,
    name: draft.name,
  }),
) {
  return useAsyncForm<Source, Draft>({
    load,
    createDraft: source => ({ name: source.name }),
    isDirty: (source, draft) => source.name !== draft.name,
    validate: draft => draft.name.trim().length > 0,
    save: ({ source, draft }) => save(source, draft),
  })
}

test('initial load does not expose defaults before the complete source is available', async () => {
  const request = deferred<Source>()
  const form = createForm(() => request.promise)

  const initializePromise = form.initialize()

  assert.equal(form.phase.value, 'loading')
  assert.equal(form.initialLoading.value, true)
  assert.equal(form.ready.value, false)
  assert.equal(form.source.value, null)
  assert.equal(form.draft.value, null)
  assert.equal(form.canSave.value, false)

  request.resolve({ id: 1, name: 'API value' })
  assert.equal(await initializePromise, true)

  assert.equal(form.phase.value, 'ready')
  assert.deepEqual(form.source.value, { id: 1, name: 'API value' })
  assert.deepEqual(form.draft.value, { name: 'API value' })
  assert.equal(form.ready.value, true)
  assert.equal(form.dirty.value, false)
})

test('initial load failure keeps the form unavailable and exposes the original error', async () => {
  const expectedError = new Error('load failed')
  const form = createForm(async () => {
    throw expectedError
  })

  assert.equal(await form.initialize(), false)
  assert.equal(form.phase.value, 'load-error')
  assert.equal(form.ready.value, false)
  assert.equal(form.source.value, null)
  assert.equal(form.draft.value, null)
  assert.equal(form.loadError.value, expectedError)
  assert.equal(form.canSave.value, false)
})

test('dirty, validation, resetDraft, and canSave are derived from the loaded snapshot', async () => {
  const form = createForm(async () => ({ id: 1, name: 'Original' }))
  await form.initialize()

  form.draft.value!.name = 'Changed'
  assert.equal(form.dirty.value, true)
  assert.equal(form.valid.value, true)
  assert.equal(form.canSave.value, true)

  form.draft.value!.name = ' '
  assert.equal(form.dirty.value, true)
  assert.equal(form.valid.value, false)
  assert.equal(form.canSave.value, false)

  assert.equal(form.resetDraft(), true)
  assert.deepEqual(form.draft.value, { name: 'Original' })
  assert.equal(form.dirty.value, false)
})

test('refresh protects a dirty draft unless discarding changes is explicit', async () => {
  let source = { id: 1, name: 'Initial' }
  const form = createForm(async () => source)
  await form.initialize()
  form.draft.value!.name = 'Unsaved'
  source = { id: 1, name: 'Fresh API value' }

  assert.equal(await form.refresh(), false)
  assert.deepEqual(form.source.value, { id: 1, name: 'Initial' })
  assert.deepEqual(form.draft.value, { name: 'Unsaved' })

  assert.equal(await form.refresh({ discardChanges: true }), true)
  assert.deepEqual(form.source.value, { id: 1, name: 'Fresh API value' })
  assert.deepEqual(form.draft.value, { name: 'Fresh API value' })
  assert.equal(form.dirty.value, false)
})

test('failed refresh retains the last good source and editable draft', async () => {
  const expectedError = new Error('refresh failed')
  let shouldFail = false
  const form = createForm(async () => {
    if (shouldFail) {
      throw expectedError
    }
    return { id: 1, name: 'Loaded' }
  })
  await form.initialize()
  shouldFail = true

  assert.equal(await form.refresh(), false)
  assert.equal(form.phase.value, 'load-error')
  assert.equal(form.ready.value, true)
  assert.deepEqual(form.source.value, { id: 1, name: 'Loaded' })
  assert.deepEqual(form.draft.value, { name: 'Loaded' })
  assert.equal(form.loadError.value, expectedError)
})

test('save disables the form action and atomically commits the returned API source', async () => {
  const request = deferred<Source>()
  const form = createForm(
    async () => ({ id: 1, name: 'Original' }),
    () => request.promise,
  )
  await form.initialize()
  form.draft.value!.name = 'Changed'

  const savePromise = form.save()
  assert.equal(form.phase.value, 'saving')
  assert.equal(form.saving.value, true)
  assert.equal(form.busy.value, true)
  assert.equal(form.canSave.value, false)

  request.resolve({ id: 1, name: 'Normalized by API' })
  assert.equal(await savePromise, true)
  assert.deepEqual(form.source.value, { id: 1, name: 'Normalized by API' })
  assert.deepEqual(form.draft.value, { name: 'Normalized by API' })
  assert.equal(form.phase.value, 'ready')
  assert.equal(form.dirty.value, false)
})

test('failed save preserves the user draft for correction and retry', async () => {
  const expectedError = new Error('save failed')
  const form = createForm(
    async () => ({ id: 1, name: 'Original' }),
    async () => {
      throw expectedError
    },
  )
  await form.initialize()
  form.draft.value!.name = 'User draft'

  assert.equal(await form.save(), false)
  assert.equal(form.phase.value, 'save-error')
  assert.deepEqual(form.source.value, { id: 1, name: 'Original' })
  assert.deepEqual(form.draft.value, { name: 'User draft' })
  assert.equal(form.saveError.value, expectedError)
  assert.equal(form.canSave.value, true)
})

test('reset invalidates a pending request so a stale response cannot repopulate the form', async () => {
  const request = deferred<Source>()
  const form = createForm(() => request.promise)
  const initializePromise = form.initialize()

  form.reset()
  request.resolve({ id: 1, name: 'Stale response' })

  assert.equal(await initializePromise, false)
  assert.equal(form.phase.value, 'idle')
  assert.equal(form.source.value, null)
  assert.equal(form.draft.value, null)
})
