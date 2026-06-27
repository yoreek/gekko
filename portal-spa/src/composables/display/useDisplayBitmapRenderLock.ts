import { ref, type Ref } from 'vue'

export interface DisplayBitmapRenderLock {
  bitmapRenderFrozen: Ref<boolean>
  setBitmapRenderLock: (selectedWidgetId: string | null, activeWidgetId: string | null, mode: 'drag' | 'resize' | null, isBitmapWidget: boolean) => void
}

export function useDisplayBitmapRenderLock(): DisplayBitmapRenderLock {
  const bitmapRenderFrozen = ref(false)

  function setBitmapRenderLock(selectedWidgetId: string | null, activeWidgetId: string | null, mode: 'drag' | 'resize' | null, isBitmapWidget: boolean): void {
    bitmapRenderFrozen.value = mode === 'resize' && isBitmapWidget && selectedWidgetId === activeWidgetId
  }

  return { bitmapRenderFrozen, setBitmapRenderLock }
}
