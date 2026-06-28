import { onBeforeUnmount } from 'vue'

import type { DisplayBitmapWidget } from '@/models/devices/display/layout'
import { cloneDisplayBitmapWidget } from '@/models/devices/display/widgets'

type BitmapResizeTransaction<TWidget extends DisplayBitmapWidget> = {
  widgetId: string
  source: TWidget
  clearTimer: ReturnType<typeof setTimeout> | null
  active: boolean
}

const BITMAP_RESIZE_TRANSACTION_DEBOUNCE_MS = 250

export interface DisplayBitmapResizeTransactionState<TWidget extends DisplayBitmapWidget> {
  beginBitmapResizeTransaction: (widgetId: string) => void
  clearBitmapResizeTransaction: () => void
  endBitmapResizeTransaction: (widgetId?: string) => void
  syncBitmapWidget: (previousWidget: TWidget | null, widget: TWidget, resizeWidget: (widget: TWidget, size: { width: number; height: number }) => TWidget) => TWidget
}

export function useDisplayBitmapResizeTransaction<TWidget extends DisplayBitmapWidget>(
  widgets: () => TWidget[],
): DisplayBitmapResizeTransactionState<TWidget> {
  let bitmapResizeTransaction: BitmapResizeTransaction<TWidget> | null = null

  onBeforeUnmount(() => {
    clearBitmapResizeTransaction()
  })

  function beginBitmapResizeTransaction(widgetId: string): void {
    const widget = widgets().find(item => item.id === widgetId)
    if (widget?.type !== 'bitmap') {
      return
    }
    if (bitmapResizeTransaction?.widgetId === widgetId) {
      if (bitmapResizeTransaction.clearTimer !== null) {
        clearTimeout(bitmapResizeTransaction.clearTimer)
        bitmapResizeTransaction.clearTimer = null
      }
      bitmapResizeTransaction.active = true
      return
    }
    clearBitmapResizeTransaction()
    bitmapResizeTransaction = {
      widgetId,
      source: cloneDisplayBitmapWidget(widget),
      clearTimer: null,
      active: true,
    }
  }

  function endBitmapResizeTransaction(widgetId?: string): void {
    if (bitmapResizeTransaction === null) {
      return
    }
    if (widgetId !== undefined && bitmapResizeTransaction.widgetId !== widgetId) {
      return
    }
    bitmapResizeTransaction.active = false
    scheduleBitmapResizeTransactionClear(bitmapResizeTransaction.widgetId)
  }

  function scheduleBitmapResizeTransactionClear(widgetId: string): void {
    if (bitmapResizeTransaction === null || bitmapResizeTransaction.widgetId !== widgetId) {
      return
    }
    if (bitmapResizeTransaction.active) {
      return
    }
    if (bitmapResizeTransaction.clearTimer !== null) {
      clearTimeout(bitmapResizeTransaction.clearTimer)
    }
    bitmapResizeTransaction.clearTimer = setTimeout(() => {
      if (bitmapResizeTransaction?.widgetId === widgetId) {
        clearBitmapResizeTransaction()
      }
    }, BITMAP_RESIZE_TRANSACTION_DEBOUNCE_MS)
  }

  function clearBitmapResizeTransaction(): void {
    if (bitmapResizeTransaction === null) {
      return
    }
    if (bitmapResizeTransaction.clearTimer !== null) {
      clearTimeout(bitmapResizeTransaction.clearTimer)
    }
    bitmapResizeTransaction = null
  }

  function resolveBitmapResizeSource(previousWidget: TWidget, widget: TWidget): TWidget {
    if (bitmapResizeTransaction?.widgetId === widget.id) {
      if (!bitmapResizeTransaction.active) {
        scheduleBitmapResizeTransactionClear(widget.id)
      }
      return bitmapResizeTransaction.source
    }
    clearBitmapResizeTransaction()
    const source = cloneDisplayBitmapWidget(previousWidget)
    bitmapResizeTransaction = {
      widgetId: widget.id,
      source,
      clearTimer: null,
      active: false,
    }
    scheduleBitmapResizeTransactionClear(widget.id)
    return source
  }

  function syncBitmapWidget(
    previousWidget: TWidget | null,
    widget: TWidget,
    resizeWidget: (widget: TWidget, size: { width: number; height: number }) => TWidget,
  ): TWidget {
    if (widget.type !== 'bitmap') {
      return widget
    }
    if (previousWidget?.type !== 'bitmap' || (previousWidget.width === widget.width && previousWidget.height === widget.height)) {
      if (previousWidget?.type === 'bitmap' && previousWidget.bitmapData !== widget.bitmapData) {
        clearBitmapResizeTransaction()
      }
      return widget
    }
    return resizeWidget(resolveBitmapResizeSource(previousWidget, widget), {
      width: widget.width,
      height: widget.height,
    })
  }

  return {
    beginBitmapResizeTransaction,
    clearBitmapResizeTransaction,
    endBitmapResizeTransaction,
    syncBitmapWidget,
  }
}
