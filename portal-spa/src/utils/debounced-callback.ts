export interface DebouncedCallback<Value> {
  schedule(value: Value): void
  cancel(): void
}

export function createDebouncedCallback<Value>(
  callback: (value: Value) => void,
  delayMs: number,
): DebouncedCallback<Value> {
  let timer: ReturnType<typeof setTimeout> | null = null

  function cancel(): void {
    if (timer === null) {
      return
    }
    clearTimeout(timer)
    timer = null
  }

  return {
    schedule(value: Value): void {
      cancel()
      timer = setTimeout(() => {
        timer = null
        callback(value)
      }, delayMs)
    },
    cancel,
  }
}
