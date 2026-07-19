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

const textDataUrl = computed(() => {
  if (props.widget.type !== 'text') return ''
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
</script>
