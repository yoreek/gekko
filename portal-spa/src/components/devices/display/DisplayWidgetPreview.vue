<template>
  <rect
    v-if="widget.type === 'rect'"
    :width="Math.max(1, widget.width)"
    :height="Math.max(1, widget.height)"
    :fill="widget.styleFlags.filled ? inkColor : 'none'"
    :stroke="inkColor"
    :stroke-width="widget.strokeWidth"
  />
  <line
    v-else-if="widget.type === 'line'"
    x1="0"
    :y1="widget.height / 2"
    :x2="widget.width"
    :y2="widget.height / 2"
    :stroke="inkColor"
    :stroke-width="widget.strokeWidth"
  />
  <ellipse
    v-else-if="widget.type === 'circle' || widget.type === 'ellipse'"
    :cx="widget.width / 2"
    :cy="widget.height / 2"
    :rx="Math.max(1, widget.width / 2)"
    :ry="Math.max(1, widget.height / 2)"
    :fill="widget.styleFlags.filled ? inkColor : 'none'"
    :stroke="inkColor"
    :stroke-width="widget.strokeWidth"
  />
  <g v-else-if="widget.type === 'character'">
    <rect
      :width="Math.max(1, widget.width)"
      :height="Math.max(1, widget.height)"
      fill="none"
      :stroke="inkColor"
      stroke-width="0"
    />
    <text
      x="0"
      y="0.78"
      text-anchor="start"
      dominant-baseline="alphabetic"
      :font-size="cellTextFontSize"
      font-family="monospace"
      :fill="inkColor"
    >
      <tspan
        v-for="(line, index) in characterLines"
        :key="index"
        x="0"
        :y="0.78 + index"
        :textLength="line.length > 1 ? Math.min(widget.width, line.length) : undefined"
        lengthAdjust="spacingAndGlyphs"
      >{{ line }}</tspan>
    </text>
  </g>
  <image
    v-else-if="widget.type === 'text'"
    :href="textDataUrl"
    x="0"
    y="0"
    :width="Math.max(1, widget.width)"
    :height="Math.max(1, widget.height)"
    preserveAspectRatio="none"
  />
  <image
    v-else-if="widget.type === 'bitmap'"
    :href="bitmapDataUrl"
    x="0"
    y="0"
    :width="Math.max(1, widget.width)"
    :height="Math.max(1, widget.height)"
    preserveAspectRatio="none"
  />
  <template v-else-if="widget.type === 'digital'">
    <defs>
      <clipPath :id="digitalClipId">
        <rect :width="Math.max(1, widget.width)" :height="Math.max(1, widget.height)" />
      </clipPath>
    </defs>
    <g :clip-path="`url(#${digitalClipId})`">
    <g
      v-for="digit in digitalDigits"
      :key="digit.index"
      :transform="`translate(${digit.x}, 0)`"
    >
      <rect
        v-for="segment in digit.segments"
        :key="segment.id"
        :x="segment.x"
        :y="segment.y"
        :width="segment.width"
        :height="segment.height"
        :rx="segment.rx"
        :fill="inkColor"
        :fill-opacity="segment.active ? 0.92 : 0.18"
      />
      <circle
        :cx="digitWidth + 0.12"
        :cy="1 - 0.08 - dotRadius"
        :r="dotRadius"
        :fill="inkColor"
        :fill-opacity="digit.dot ? 0.98 : 0.5"
      />
    </g>
    </g>
  </template>
</template>

<script setup lang="ts">
import { computed } from 'vue'

import type { BaseDisplay } from '@/models/devices/display/display'
import type { DisplayWidget } from '@/models/devices/display/layout'
import { classicFontScale, drawClassicFontText } from '@/models/devices/display/text/classic-font'
import type { RasterImageFormat } from '@/raster/raster-image-types'

const props = defineProps<{
  widget: DisplayWidget
  display: BaseDisplay<RasterImageFormat>
  backgroundColor: string
  previewText?: string
  freezeRender?: boolean
}>()

const inkColor = computed(() => (props.widget.styleFlags.inverted ? props.backgroundColor : props.widget.color))
const textColor = computed(() => (props.widget.styleFlags.inverted ? props.backgroundColor : props.widget.color))
const isCellDisplay = computed(() => props.display.coordinateUnit === 'cell')
const digitalText = computed(() => (props.previewText ?? props.widget.text).trim())
const digitWidth = computed(() => 0.72)
const digitHeight = computed(() => 0.9)
const digitalClipId = computed(() => `display-digital-clip-${props.widget.id.replace(/[^a-zA-Z0-9_-]/g, '-')}`)
const dotRadius = computed(() => Math.max(0.1, Math.min(digitWidth.value, digitHeight.value) * 0.1))
const cellTextFontSize = computed(() => Math.max(0.55, Math.min(0.9, props.widget.height * 0.72)))
const characterLines = computed(() => {
  const width = Math.max(1, props.widget.width)
  const height = Math.max(1, props.widget.height)
  const sourceLines = (props.previewText ?? props.widget.text).split('\n')
  const lines: string[] = []
  for (const sourceLine of sourceLines) {
    if (props.widget.styleFlags.wrap) {
      for (let offset = 0; offset < Math.max(1, sourceLine.length); offset += width) {
        lines.push(sourceLine.slice(offset, offset + width))
      }
    } else {
      lines.push(sourceLine.slice(0, width))
    }
  }
  return lines.slice(0, height)
})

type DigitalDigit = {
  index: number
  x: number
  dot: boolean
  segments: Array<{ id: string; x: number; y: number; width: number; height: number; rx: number; active: boolean }>
}

// Mirrors DisplayDigitalFormatter.cpp's buildDisplayDigitalFrame(): a value that doesn't resolve
// (empty) or doesn't fit the cell count is replaced by a fallback pattern rather than silently
// truncated - firmware calls this "----" (digitalMissing/digitalOverflow aren't modeled in the SPA
// layout yet, so this always uses firmware's own struct default for both). A value that fits is
// right-aligned, matching the widget's DisplayDigitalAlign::Right struct default (also not yet
// exposed as an editable field here).
const kDigitalFallbackPattern = '----'

function fillDigitalPattern(cellCount: number, pattern: string): Array<{ glyph: string; dot: boolean }> {
  const source = pattern.length > 0 ? pattern : kDigitalFallbackPattern
  return Array.from({ length: cellCount }, (_, index) => ({ glyph: source[index % source.length] ?? '-', dot: false }))
}

const digitalDigits = computed<DigitalDigit[]>(() => {
  const cellCount = Math.max(1, props.widget.width)
  const parsed = parseDigitalPreviewText(digitalText.value)
  let segments: Array<{ glyph: string; dot: boolean }>
  if (digitalText.value.length === 0 || parsed.length > cellCount) {
    segments = fillDigitalPattern(cellCount, kDigitalFallbackPattern)
  } else {
    const padCount = cellCount - parsed.length
    segments = [...Array.from({ length: padCount }, () => ({ glyph: ' ', dot: false })), ...parsed]
  }
  return segments.map((cell, index) => {
    const x = index + 0.04
    const w = digitWidth.value
    const h = digitHeight.value
    const thickness = Math.max(0.12, Math.min(w, h) * 0.16)
    const inset = Math.max(0.02, Math.min(w, h) * 0.02)
    const horizontalWidth = Math.max(0.2, w - inset * 2 - thickness * 1.6)
    const verticalHeight = Math.max(0.2, h / 2 - thickness * 1.2)
    const active = segmentPatternForGlyph(cell.glyph)
    return {
      index,
      x,
      dot: cell.dot,
      segments: [
        { id: 'a', x: inset + thickness * 0.6, y: inset, width: horizontalWidth, height: thickness, rx: thickness / 2, active: active.includes('a') },
        { id: 'b', x: w - thickness - inset, y: inset + thickness * 0.7, width: thickness, height: verticalHeight, rx: thickness / 2, active: active.includes('b') },
        { id: 'c', x: w - thickness - inset, y: h / 2 + thickness * 0.3, width: thickness, height: verticalHeight, rx: thickness / 2, active: active.includes('c') },
        { id: 'd', x: inset + thickness * 0.6, y: h - thickness - inset, width: horizontalWidth, height: thickness, rx: thickness / 2, active: active.includes('d') },
        { id: 'e', x: inset, y: h / 2 + thickness * 0.3, width: thickness, height: verticalHeight, rx: thickness / 2, active: active.includes('e') },
        { id: 'f', x: inset, y: inset + thickness * 0.7, width: thickness, height: verticalHeight, rx: thickness / 2, active: active.includes('f') },
        { id: 'g', x: inset + thickness * 0.6, y: h / 2 - thickness / 2, width: horizontalWidth, height: thickness, rx: thickness / 2, active: active.includes('g') },
      ],
    }
  })
})

const textDataUrl = computed(() => {
  if (props.widget.type !== 'text' || isCellDisplay.value) return ''
  const canvas = document.createElement('canvas')
  canvas.width = Math.max(1, Math.round(props.widget.width))
  canvas.height = Math.max(1, Math.round(props.widget.height))
  const ctx = canvas.getContext('2d')
  if (ctx === null) return ''
  const scale = classicFontScale(props.widget.fontSize)
  drawClassicFontText(ctx, props.previewText ?? props.widget.text, {
    scale,
    wrap: props.widget.styleFlags.wrap,
    maxWidth: canvas.width,
    maxHeight: canvas.height,
    color: props.widget.styleFlags.inverted ? props.backgroundColor : props.widget.color,
    backgroundColor: props.widget.color,
    clear: props.widget.styleFlags.inverted,
  })
  return canvas.toDataURL()
})

const bitmapDataUrl = computed(() => {
  if (props.widget.type !== 'bitmap' || props.freezeRender === true) return ''
  const canvas = document.createElement('canvas')
  canvas.width = Math.max(1, Math.round(props.widget.width))
  canvas.height = Math.max(1, Math.round(props.widget.height))
  props.display.renderWidget(props.widget, canvas)
  return canvas.toDataURL()
})

function parseDigitalPreviewText(text: string): Array<{ glyph: string; dot: boolean }> {
  const cells: Array<{ glyph: string; dot: boolean }> = []
  for (const character of text) {
    if (character === '.') {
      if (cells.length > 0) {
        cells[cells.length - 1].dot = true
      }
      continue
    }
    if (character === ' ') {
      cells.push({ glyph: ' ', dot: false })
      continue
    }
    cells.push({ glyph: character, dot: false })
  }
  return cells
}

function segmentPatternForGlyph(glyph: string): string[] {
  switch (glyph.toUpperCase()) {
    case '0':
      return ['a', 'b', 'c', 'd', 'e', 'f']
    case '1':
      return ['b', 'c']
    case '2':
      return ['a', 'b', 'g', 'e', 'd']
    case '3':
      return ['a', 'b', 'g', 'c', 'd']
    case '4':
      return ['f', 'g', 'b', 'c']
    case '5':
      return ['a', 'f', 'g', 'c', 'd']
    case '6':
      return ['a', 'f', 'g', 'c', 'd', 'e']
    case '7':
      return ['a', 'b', 'c']
    case '8':
      return ['a', 'b', 'c', 'd', 'e', 'f', 'g']
    case '9':
      return ['a', 'b', 'c', 'd', 'f', 'g']
    case '-':
      return ['g']
    default:
      return []
  }
}
</script>
