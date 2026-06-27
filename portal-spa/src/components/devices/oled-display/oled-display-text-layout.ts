import { classicFontScale, measureClassicFontText } from './classic-font.ts'
import type { OledDisplayWidget } from '@/models/devices/oled-display-layout'

export interface OledDisplayTextLayoutMeasurement {
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

export function measureOledDisplayTextWidget(widget: OledDisplayWidget): OledDisplayTextLayoutMeasurement {
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

export function autoSizeOledDisplayTextWidget(
  widget: Omit<OledDisplayWidget, 'width' | 'height'> & { width: number; height: number },
  layoutWidth: number,
  layoutHeight: number,
): OledDisplayWidget {
  if (widget.type !== 'text' || !widget.autoSize) {
    return widget as OledDisplayWidget
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
  } as OledDisplayWidget
}
