import {
  computed,
  getCurrentScope,
  onScopeDispose,
  readonly,
  ref,
  shallowRef,
  type Ref,
} from 'vue'

export type AsyncFormPhase =
  | 'idle'
  | 'loading'
  | 'ready'
  | 'refreshing'
  | 'saving'
  | 'load-error'
  | 'save-error'

export type AsyncFormCommitReason = 'load' | 'refresh' | 'save'

export interface AsyncFormLoadContext {
  signal: AbortSignal
}

export interface AsyncFormSaveContext<TSource, TDraft> {
  source: TSource
  draft: TDraft
  signal: AbortSignal
}

export interface AsyncFormOptions<TSource, TDraft> {
  load(context: AsyncFormLoadContext): Promise<TSource>
  createDraft(source: TSource): TDraft
  isDirty(source: TSource, draft: TDraft): boolean
  save(context: AsyncFormSaveContext<TSource, TDraft>): Promise<TSource>
  validate?(draft: TDraft): boolean
  onCommit?(source: TSource, reason: AsyncFormCommitReason): void
}

export interface AsyncFormRefreshOptions {
  discardChanges?: boolean
}

export function useAsyncForm<TSource, TDraft>(options: AsyncFormOptions<TSource, TDraft>) {
  const source = shallowRef<TSource | null>(null)
  const draft = ref<TDraft | null>(null) as Ref<TDraft | null>
  const phase = ref<AsyncFormPhase>('idle')
  const loadError = shallowRef<unknown | null>(null)
  const saveError = shallowRef<unknown | null>(null)

  let operationSequence = 0
  let activeController: AbortController | null = null

  const initialLoading = computed(() => phase.value === 'loading')
  const refreshing = computed(() => phase.value === 'refreshing')
  const saving = computed(() => phase.value === 'saving')
  const busy = computed(() => initialLoading.value || refreshing.value || saving.value)
  const ready = computed(() => source.value !== null && draft.value !== null)
  const dirty = computed(() => (
    source.value !== null
    && draft.value !== null
    && options.isDirty(source.value, draft.value)
  ))
  const valid = computed(() => (
    draft.value !== null
    && (options.validate?.(draft.value) ?? true)
  ))
  const canSave = computed(() => ready.value && dirty.value && valid.value && !busy.value)

  function isCurrentOperation(sequence: number): boolean {
    return sequence === operationSequence
  }

  function beginOperation(): { controller: AbortController; sequence: number } {
    activeController?.abort()
    const controller = new AbortController()
    activeController = controller
    operationSequence += 1
    return { controller, sequence: operationSequence }
  }

  function finishOperation(sequence: number): void {
    if (isCurrentOperation(sequence)) {
      activeController = null
    }
  }

  function commit(nextSource: TSource, reason: AsyncFormCommitReason): void {
    // Build the draft before publishing either value so consumers never observe a source without
    // its matching editable model. onCommit is part of the same transaction: if it rejects the
    // source (for example while synchronizing a store), the form remains on its previous snapshot.
    const nextDraft = options.createDraft(nextSource)
    options.onCommit?.(nextSource, reason)
    source.value = nextSource
    draft.value = nextDraft
  }

  async function runLoad(reason: 'load' | 'refresh'): Promise<boolean> {
    if (busy.value) {
      return false
    }

    const { controller, sequence } = beginOperation()
    phase.value = reason === 'load' ? 'loading' : 'refreshing'
    loadError.value = null

    try {
      const nextSource = await options.load({ signal: controller.signal })
      if (!isCurrentOperation(sequence) || controller.signal.aborted) {
        return false
      }
      commit(nextSource, reason)
      saveError.value = null
      phase.value = 'ready'
      return true
    } catch (error) {
      if (!isCurrentOperation(sequence) || controller.signal.aborted) {
        return false
      }
      loadError.value = error
      phase.value = 'load-error'
      return false
    } finally {
      finishOperation(sequence)
    }
  }

  async function initialize(): Promise<boolean> {
    if (ready.value) {
      return true
    }
    return runLoad('load')
  }

  async function refresh(refreshOptions: AsyncFormRefreshOptions = {}): Promise<boolean> {
    if (!ready.value) {
      return initialize()
    }
    if (dirty.value && refreshOptions.discardChanges !== true) {
      return false
    }
    return runLoad('refresh')
  }

  async function save(): Promise<boolean> {
    if (!canSave.value || source.value === null || draft.value === null) {
      return false
    }

    const sourceToSave = source.value
    const draftToSave = draft.value
    const { controller, sequence } = beginOperation()
    phase.value = 'saving'
    saveError.value = null

    try {
      const nextSource = await options.save({
        source: sourceToSave,
        draft: draftToSave,
        signal: controller.signal,
      })
      if (!isCurrentOperation(sequence) || controller.signal.aborted) {
        return false
      }
      commit(nextSource, 'save')
      loadError.value = null
      phase.value = 'ready'
      return true
    } catch (error) {
      if (!isCurrentOperation(sequence) || controller.signal.aborted) {
        return false
      }
      saveError.value = error
      phase.value = 'save-error'
      return false
    } finally {
      finishOperation(sequence)
    }
  }

  function resetDraft(): boolean {
    if (source.value === null) {
      return false
    }
    draft.value = options.createDraft(source.value)
    saveError.value = null
    if (phase.value === 'save-error') {
      phase.value = 'ready'
    }
    return true
  }

  function clearErrors(): void {
    loadError.value = null
    saveError.value = null
    if (phase.value === 'load-error' || phase.value === 'save-error') {
      phase.value = ready.value ? 'ready' : 'idle'
    }
  }

  function reset(): void {
    activeController?.abort()
    activeController = null
    operationSequence += 1
    source.value = null
    draft.value = null
    loadError.value = null
    saveError.value = null
    phase.value = 'idle'
  }

  if (getCurrentScope() !== undefined) {
    onScopeDispose(() => {
      activeController?.abort()
      activeController = null
      operationSequence += 1
    })
  }

  return {
    source: readonly(source),
    draft,
    phase: readonly(phase),
    loadError: readonly(loadError),
    saveError: readonly(saveError),
    initialLoading,
    refreshing,
    saving,
    busy,
    ready,
    dirty,
    valid,
    canSave,
    initialize,
    refresh,
    save,
    resetDraft,
    clearErrors,
    reset,
  }
}
