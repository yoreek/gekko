import { classicFontScale, measureClassicFontText } from './classic-font.ts'
import type { Ssd1306Widget } from '@/models/devices/ssd1306/layout'

export interface Ssd1306TextLayoutMeasurement {
  scale: number
  measuredWidth: number
  measuredHeight: number
  lineHeight: number
  lineCount: number
  columnsPerLine: number
  boxWidth: number
  boxHeight: number
  fits: boolean
  wrappedLines: number
}

export function measureSsd1306TextWidget(widget: Ssd1306Widget): Ssd1306TextLayoutMeasurement {
  const scale = classicFontScale(widget.fontSize)
  const boxWidth = Math.max(1, Math.round(widget.width))
  const boxHeight = Math.max(1, Math.round(widget.height))
  const measured = measureClassicFontText(widget.text, scale, widget.styleFlags.wrap, widget.styleFlags.wrap ? boxWidth : Number.POSITIVE_INFINITY)
  return {
    scale,
    measuredWidth: measured.width,
    measuredHeight: measured.height,
    lineHeight: measured.lineHeight,
    lineCount: measured.lineCount,
    columnsPerLine: measured.columnsPerLine,
    boxWidth,
    boxHeight,
    fits: measured.width <= boxWidth && measured.height <= boxHeight,
    wrappedLines: measured.lineCount,
  }
}

export function autoSizeSsd1306TextWidget(
  widget: Omit<Ssd1306Widget, 'width' | 'height'> & { width: number; height: number },
  layoutWidth: number,
  layoutHeight: number,
): Ssd1306Widget {
  if (widget.type !== 'text' || !widget.autoSize) {
    return widget as Ssd1306Widget
  }

  const scale = classicFontScale(widget.fontSize)
  const availableWidth = Math.max(1, layoutWidth - Math.max(0, Math.round(widget.x)))
  const availableHeight = Math.max(1, layoutHeight - Math.max(0, Math.round(widget.y)))
  const unwrapped = measureClassicFontText(widget.text, scale, false)
  const width = Math.max(1, Math.min(availableWidth, unwrapped.width))
  const sized = measureClassicFontText(widget.text, scale, widget.styleFlags.wrap, width)
  return {
    ...widget,
    width,
    height: Math.max(1, Math.min(availableHeight, sized.height)),
  } as Ssd1306Widget
}
