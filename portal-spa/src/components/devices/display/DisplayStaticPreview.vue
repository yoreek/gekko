<template>
  <v-sheet
    border
    rounded
    class="pa-3 d-flex align-center justify-center"
  >
    <svg
      :width="displayWidth"
      :height="displayHeight"
      :viewBox="`0 0 ${viewBoxWidth} ${viewBoxHeight}`"
      role="img"
      :aria-label="ariaLabel"
    >
      <rect
        x="0"
        y="0"
        :width="viewBoxWidth"
        :height="viewBoxHeight"
        fill="rgb(var(--v-theme-surface-variant))"
        stroke="rgb(var(--v-theme-outline))"
        vector-effect="non-scaling-stroke"
      />

      <g v-if="isGraphicDisplay" :transform="contentTransform">
        <rect
          :x="graphicInset"
          :y="graphicInset"
          :width="graphicContentWidth"
          :height="graphicContentHeight"
          rx="4"
          fill="rgb(var(--v-theme-surface))"
          stroke="rgb(var(--v-theme-outline-variant))"
          vector-effect="non-scaling-stroke"
        />
        <path
          :d="`M${graphicInset},${graphicInset + originMarkerSize} L${graphicInset},${graphicInset} L${graphicInset + originMarkerSize},${graphicInset}`"
          fill="none"
          stroke="rgb(var(--v-theme-primary))"
          stroke-width="2"
          vector-effect="non-scaling-stroke"
        />
        <text
          :x="viewBoxWidth / 2"
          :y="viewBoxHeight / 2 - graphicTextOffset"
          text-anchor="middle"
          dominant-baseline="central"
          :font-size="graphicTitleSize"
          fill="rgb(var(--v-theme-on-surface))"
          font-weight="600"
        >Aa</text>
        <text
          :x="viewBoxWidth / 2"
          :y="viewBoxHeight / 2 + graphicTextOffset"
          text-anchor="middle"
          dominant-baseline="central"
          :font-size="graphicSubtitleSize"
          fill="rgb(var(--v-theme-on-surface-variant))"
          font-weight="500"
        >12.3 V</text>
      </g>

      <g v-else-if="isLcdDisplay">
        <rect
          v-for="row in lcdRows"
          :key="row.index"
          :x="lcdInset"
          :y="row.y"
          :width="lcdContentWidth"
          :height="lcdRowHeight"
          rx="3"
          fill="rgb(var(--v-theme-surface))"
          stroke="rgb(var(--v-theme-outline-variant))"
          vector-effect="non-scaling-stroke"
        />
        <text
          v-for="row in lcdRows"
          :key="`${row.index}-text`"
          :x="viewBoxWidth / 2"
          :y="row.textY"
          text-anchor="middle"
          dominant-baseline="central"
          :font-size="lcdFontSize"
          fill="rgb(var(--v-theme-on-surface))"
          font-weight="500"
          font-family="monospace"
          :transform="contentTransform"
        >{{ row.text }}</text>
      </g>

      <g v-else>
        <g
          v-for="digit in tmDigits"
          :key="digit.index"
          :transform="`translate(${digit.x}, ${digit.y}) ${tmRotationTransform}`"
        >
          <rect
            :x="0"
            :y="0"
            :width="tmDigitWidth"
            :height="tmDigitHeight"
            rx="4"
            fill="rgb(var(--v-theme-surface))"
            stroke="rgb(var(--v-theme-outline-variant))"
            vector-effect="non-scaling-stroke"
          />
          <rect
            v-for="segment in digit.segments"
            :key="segment.id"
            :x="segment.x"
            :y="segment.y"
            :width="segment.width"
            :height="segment.height"
            :rx="segment.rx"
            fill="rgb(var(--v-theme-primary))"
            :fill-opacity="segment.active ? 0.92 : 0.18"
          />
          <circle
            :cx="tmDigitWidth / 2"
            :cy="tmDotY"
            :r="tmDotRadius"
            fill="rgb(var(--v-theme-primary))"
            :fill-opacity="0.28"
          />
        </g>
      </g>
    </svg>
  </v-sheet>
</template>

<script setup lang="ts">
import { computed } from 'vue'

type PreviewKind = 'graphic' | 'lcd1602' | 'lcd2004' | 'tm1637'

type SegmentDefinition = {
  id: string
  x: number
  y: number
  width: number
  height: number
  rx: number
  active: boolean
}

type DigitDefinition = {
  index: number
  x: number
  y: number
  segments: SegmentDefinition[]
}

const props = withDefaults(defineProps<{
  kind: PreviewKind
  width?: number
  height?: number
  rotation?: number
  maxSide?: number
}>(), {
  maxSide: 128,
  rotation: 0,
})

const isGraphicDisplay = computed(() => props.kind === 'graphic')
const isLcdDisplay = computed(() => props.kind === 'lcd1602' || props.kind === 'lcd2004')

const lcdRows = computed(() => {
  if (props.kind === 'lcd2004') {
    return [
      { index: 0, text: '12.3 V', y: 10, textY: 16 },
      { index: 1, text: '23.5 C', y: 27, textY: 33 },
      { index: 2, text: 'PUMP 42%', y: 44, textY: 50 },
      { index: 3, text: 'READY', y: 61, textY: 67 },
    ]
  }
  return [
    { index: 0, text: '12.3 V', y: 14, textY: 22 },
    { index: 1, text: '23.5 C', y: 40, textY: 48 },
  ]
})

const tmDigits = computed<DigitDefinition[]>(() => {
  const digitPatterns = [
    { value: 1, active: ['b', 'c'] },
    { value: 2, active: ['a', 'b', 'g', 'e', 'd'] },
    { value: 3, active: ['a', 'b', 'g', 'c', 'd'] },
    { value: 4, active: ['f', 'g', 'b', 'c'] },
  ]
  return digitPatterns.map((pattern, index) => {
    const x = 11 + index * 26
    const y = 10
    return {
      index,
      x,
      y,
      segments: segmentDefinitions(pattern.active),
    }
  })
})

const viewBoxWidth = computed(() => {
  if (props.kind === 'tm1637') {
    return 115
  }
  if (props.kind === 'lcd2004') {
    return 132
  }
  if (props.kind === 'lcd1602') {
    return 112
  }
  return Math.max(1, Math.round(props.width ?? 128))
})

const viewBoxHeight = computed(() => {
  if (props.kind === 'tm1637') {
    return 48
  }
  if (props.kind === 'lcd2004') {
    return 78
  }
  if (props.kind === 'lcd1602') {
    return 58
  }
  return Math.max(1, Math.round(props.height ?? 64))
})

const displayWidth = computed(() => {
  const scale = props.maxSide / Math.max(viewBoxWidth.value, viewBoxHeight.value)
  return Math.max(1, Math.round(viewBoxWidth.value * scale))
})

const displayHeight = computed(() => {
  const scale = props.maxSide / Math.max(viewBoxWidth.value, viewBoxHeight.value)
  return Math.max(1, Math.round(viewBoxHeight.value * scale))
})

const contentTransform = computed(() => {
  if (props.kind === 'tm1637') {
    return `rotate(${props.rotation === 180 ? 180 : 0}, ${viewBoxWidth.value / 2}, ${viewBoxHeight.value / 2})`
  }
  if (props.kind === 'graphic') {
    return `rotate(${(props.rotation ?? 0) * 90}, ${viewBoxWidth.value / 2}, ${viewBoxHeight.value / 2})`
  }
  return ''
})

const tmRotationTransform = computed(() => {
  const cx = tmDigitWidth / 2
  const cy = tmDigitHeight / 2
  return props.rotation === 180 ? `rotate(180, ${cx}, ${cy})` : ''
})

const ariaLabel = computed(() => {
  if (props.kind === 'graphic') {
    return `Graphic display preview ${viewBoxWidth.value} by ${viewBoxHeight.value}`
  }
  if (props.kind === 'lcd1602') {
    return 'LCD1602 static preview'
  }
  if (props.kind === 'lcd2004') {
    return 'LCD2004 static preview'
  }
  return 'TM1637 static preview'
})

const graphicInset = computed(() => Math.max(6, Math.round(Math.min(viewBoxWidth.value, viewBoxHeight.value) * 0.12)))
const graphicContentWidth = computed(() => viewBoxWidth.value - graphicInset.value * 2)
const graphicContentHeight = computed(() => viewBoxHeight.value - graphicInset.value * 2)
const graphicTitleSize = computed(() => Math.max(10, Math.round(Math.min(viewBoxWidth.value, viewBoxHeight.value) * 0.26)))
const graphicSubtitleSize = computed(() => Math.max(8, Math.round(Math.min(viewBoxWidth.value, viewBoxHeight.value) * 0.11)))
const graphicTextOffset = computed(() => Math.max(8, Math.round(Math.min(viewBoxWidth.value, viewBoxHeight.value) * 0.18)))
const originMarkerSize = computed(() => Math.max(4, Math.round(Math.min(viewBoxWidth.value, viewBoxHeight.value) * 0.1)))

const lcdInset = computed(() => 8)
const lcdContentWidth = computed(() => viewBoxWidth.value - lcdInset.value * 2)
const lcdRowHeight = computed(() => props.kind === 'lcd2004' ? 12 : 16)
const lcdFontSize = computed(() => props.kind === 'lcd2004' ? 7 : 8)

const tmDigitWidth = 19
const tmDigitHeight = 28
const tmDotY = 32
const tmDotRadius = 1.6

function segmentDefinitions(activeSegments: string[]): SegmentDefinition[] {
  return [
    { id: 'a', x: 4, y: 1, width: 11, height: 3, rx: 1.5, active: activeSegments.includes('a') },
    { id: 'b', x: 15, y: 4, width: 3, height: 9, rx: 1.5, active: activeSegments.includes('b') },
    { id: 'c', x: 15, y: 15, width: 3, height: 9, rx: 1.5, active: activeSegments.includes('c') },
    { id: 'd', x: 4, y: 24, width: 11, height: 3, rx: 1.5, active: activeSegments.includes('d') },
    { id: 'e', x: 1, y: 15, width: 3, height: 9, rx: 1.5, active: activeSegments.includes('e') },
    { id: 'f', x: 1, y: 4, width: 3, height: 9, rx: 1.5, active: activeSegments.includes('f') },
    { id: 'g', x: 4, y: 12, width: 11, height: 3, rx: 1.5, active: activeSegments.includes('g') },
  ]
}
</script>
