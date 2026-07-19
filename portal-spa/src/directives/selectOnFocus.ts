import type { Directive } from 'vue'

type SelectableField = HTMLInputElement | HTMLTextAreaElement

function findInput(el: HTMLElement): SelectableField | null {
  if (el instanceof HTMLInputElement || el instanceof HTMLTextAreaElement) {
    return el
  }

  return el.querySelector('input, textarea')
}

type SelectOnFocusElement = HTMLElement & {
  __selectOnFocusCleanup__?: () => void
}

export const selectOnFocus: Directive<HTMLElement> = {
  mounted(el) {
    const input = findInput(el)
    if (input === null) {
      return
    }

    const handleFocus = () => {
      requestAnimationFrame(() => input.select())
    }

    input.addEventListener('focus', handleFocus)
    const shouldSuppressMouseUp = input.type !== 'number'
    const handleMouseUp = shouldSuppressMouseUp
      ? (event: Event) => {
          event.preventDefault()
        }
      : null

    if (handleMouseUp !== null) {
      input.addEventListener('mouseup', handleMouseUp)
    }

    ;(el as SelectOnFocusElement).__selectOnFocusCleanup__ = () => {
      input.removeEventListener('focus', handleFocus)
      if (handleMouseUp !== null) {
        input.removeEventListener('mouseup', handleMouseUp)
      }
    }
  },
  unmounted(el) {
    const element = el as SelectOnFocusElement
    element.__selectOnFocusCleanup__?.()
    delete element.__selectOnFocusCleanup__
  },
}
