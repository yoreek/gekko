import type { Ssd1306Widget } from '@/models/devices/ssd1306/layout'

export type DisplayCanvasStyle = Record<string, string>
export type DisplayWidgetFrameStyle = Record<string, string>

export interface DisplayCanvasBitmapSize {
  cssWidth: number
  cssHeight: number
  bitmapWidth: number
  bitmapHeight: number
  bitmapScaleX: number
  bitmapScaleY: number
}

export interface DisplayWidgetBitmapSize {
  cssWidth: number
  cssHeight: number
  bitmapWidth: number
  bitmapHeight: number
}

export function resolveDisplayCanvasStyle(deviceWidth: number, deviceHeight: number, displayScale: number): DisplayCanvasStyle {
  const scale = Math.max(1, displayScale)
  return {
    width: `${Math.max(1, deviceWidth) * scale}px`,
    height: `${Math.max(1, deviceHeight) * scale}px`,
    backgroundSize: `${8 * scale}px ${8 * scale}px`,
  }
}

export function resolveDisplayWidgetFrameStyle(widget: { x: number; y: number; width: number; height: number }, displayScale: number): DisplayWidgetFrameStyle {
  const scale = Math.max(1, displayScale)
  return {
    left: `${widget.x * scale}px`,
    top: `${widget.y * scale}px`,
    width: `${Math.max(widget.width, 1) * scale}px`,
    height: `${Math.max(widget.height, 1) * scale}px`,
  }
}

export function resolveDisplayCanvasBitmapSize(cssWidth: number, cssHeight: number, devicePixelRatio = 1): DisplayCanvasBitmapSize {
  const width = normalizePositiveCanvasSize(cssWidth)
  const height = normalizePositiveCanvasSize(cssHeight)
  const ratio = Math.max(1, devicePixelRatio)
  const bitmapWidth = Math.max(1, Math.ceil(width * ratio))
  const bitmapHeight = Math.max(1, Math.ceil(height * ratio))
  return {
    cssWidth: width,
    cssHeight: height,
    bitmapWidth,
    bitmapHeight,
    bitmapScaleX: bitmapWidth / width,
    bitmapScaleY: bitmapHeight / height,
  }
}

export function resolveDisplayWidgetBitmapSize(widgetWidth: number, widgetHeight: number): DisplayWidgetBitmapSize {
  const width = Math.max(1, Math.round(widgetWidth))
  const height = Math.max(1, Math.round(widgetHeight))
  return { cssWidth: width, cssHeight: height, bitmapWidth: width, bitmapHeight: height }
}

export function resolveDisplayTextRenderScale(fontSize: number, displayScale = 1): number {
  return Math.max(1, fontSize) * Math.max(1, displayScale)
}

export function resolveDisplayIconSize(displayScale = 1): number {
  return Math.max(12, Math.round(18 * Math.max(1, displayScale)))
}

export function resolveDisplayWidgetSpawnPosition(
  index: number,
  layoutWidth: number,
  layoutHeight: number,
  widgetWidth: number,
  widgetHeight: number,
): { x: number; y: number } {
  const offset = Math.max(0, index) * 8
  return {
    x: Math.max(0, Math.min(Math.max(0, layoutWidth - widgetWidth), offset)),
    y: Math.max(0, Math.min(Math.max(0, layoutHeight - widgetHeight), offset)),
  }
}

export function resolveDisplayWidgetDuplicatePosition(
  x: number,
  y: number,
  widgetWidth: number,
  widgetHeight: number,
  layoutWidth: number,
  layoutHeight: number,
): { x: number; y: number } {
  return {
    x: Math.max(0, Math.min(x + 4, Math.max(0, layoutWidth - widgetWidth))),
    y: Math.max(0, Math.min(y + 4, Math.max(0, layoutHeight - widgetHeight))),
  }
}

function normalizePositiveCanvasSize(value: number): number {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return 1
  }
  return Math.max(1, numeric)
}
