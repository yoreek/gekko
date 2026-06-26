import { classicFontScale } from './classic-font.ts'

export type OledDisplayCanvasStyle = Record<string, string>
export type OledDisplayWidgetFrameStyle = Record<string, string>

export interface OledDisplayCanvasBitmapSize {
  cssWidth: number
  cssHeight: number
  bitmapWidth: number
  bitmapHeight: number
  bitmapScaleX: number
  bitmapScaleY: number
}

export interface OledDisplayWidgetBitmapSize {
  cssWidth: number
  cssHeight: number
  bitmapWidth: number
  bitmapHeight: number
}

/**
 * Keep display scaling in one place so the grid, the preview, and the inspector
 * all derive the same pixel geometry from the same zoom value.
 */
export function resolveOledDisplayCanvasStyle(deviceWidth: number, deviceHeight: number, displayScale: number): OledDisplayCanvasStyle {
  const scale = Math.max(1, displayScale)
  return {
    width: `${Math.max(1, deviceWidth) * scale}px`,
    height: `${Math.max(1, deviceHeight) * scale}px`,
    backgroundSize: `${8 * scale}px ${8 * scale}px`,
  }
}

/**
 * Convert widget grid coordinates into CSS pixels for the read-only layout preview.
 */
export function resolveOledDisplayWidgetFrameStyle(widget: { x: number; y: number; width: number; height: number }, displayScale: number): OledDisplayWidgetFrameStyle {
  const scale = Math.max(1, displayScale)
  return {
    left: `${widget.x * scale}px`,
    top: `${widget.y * scale}px`,
    width: `${Math.max(widget.width, 1) * scale}px`,
    height: `${Math.max(widget.height, 1) * scale}px`,
  }
}

/**
 * Convert fractional DOM dimensions into a canvas bitmap without changing the
 * visual CSS scale. The bitmap can be rounded up, but the drawing transform must
 * map it back to the original CSS box so toggling wrap does not resize glyphs.
 */
export function resolveOledDisplayCanvasBitmapSize(cssWidth: number, cssHeight: number, devicePixelRatio = 1): OledDisplayCanvasBitmapSize {
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

/**
 * Text widgets render into OLED logical pixels first. The surrounding preview
 * surface can scale the canvas visually, but wrap and glyph size stay tied to
 * the real widget box.
 */
export function resolveOledDisplayWidgetBitmapSize(widgetWidth: number, widgetHeight: number): OledDisplayWidgetBitmapSize {
  const width = Math.max(1, Math.round(widgetWidth))
  const height = Math.max(1, Math.round(widgetHeight))
  return {
    cssWidth: width,
    cssHeight: height,
    bitmapWidth: width,
    bitmapHeight: height,
  }
}

function normalizePositiveCanvasSize(value: number): number {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return 1
  }
  return Math.max(1, numeric)
}

/**
 * Font drawing needs the widget font size plus the target surface scale.
 */
export function resolveOledDisplayTextRenderScale(fontSize: number, displayScale = 1): number {
  return classicFontScale(fontSize) * Math.max(1, displayScale)
}

/**
 * Icon previews scale with the same target surface scale as text.
 */
export function resolveOledDisplayIconSize(displayScale = 1): number {
  return Math.max(12, Math.round(18 * Math.max(1, displayScale)))
}

/**
 * Add new widgets in a predictable stagger so the first few are easy to see.
 */
export function resolveOledDisplayWidgetSpawnPosition(
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

/**
 * Duplicate widgets with a small diagonal offset, then clamp back into the layout.
 */
export function resolveOledDisplayWidgetDuplicatePosition(
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
