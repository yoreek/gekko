<template>
  <svg
    :width="deviceWidth * zoom"
    :height="deviceHeight * zoom"
    :viewBox="`0 0 ${deviceWidth} ${deviceHeight}`"
    role="application"
  >
    <rect x="0" y="0" :width="deviceWidth" :height="deviceHeight" fill="rgb(0 0 0)" />

    <g v-for="widget in widgets" :key="widget.id" :transform="`translate(${widget.x}, ${widget.y})`">
      <g
        role="button"
        tabindex="0"
        @pointerdown="startDrag($event, widget)"
        @click="$emit('select-widget', widget.id)"
        @keydown.enter.prevent="$emit('select-widget', widget.id)"
        @keydown.space.prevent="$emit('select-widget', widget.id)"
      >
        <rect :width="Math.max(1, widget.width)" :height="Math.max(1, widget.height)" fill="transparent" />
        <DisplayWidgetPreview
          :widget="widget"
          :display="display"
          :preview-text="widgetPreviewText(widget)"
          :freeze-render="isBitmapResizeActive(widget.id, widget.type)"
        />
      </g>
      <rect
        v-if="widget.id === selectedWidgetId"
        :width="Math.max(1, widget.width)"
        :height="Math.max(1, widget.height)"
        fill="none"
        stroke="rgb(var(--v-theme-primary))"
        stroke-width="1"
        pointer-events="none"
      />
      <rect
        v-if="widget.id === selectedWidgetId"
        :x="Math.max(0, widget.width - 6)"
        :y="Math.max(0, widget.height - 6)"
        width="6"
        height="6"
        fill="rgb(var(--v-theme-primary))"
        @pointerdown.stop="startResize($event, widget)"
      />
    </g>
  </svg>
</template>

<script setup lang="ts">
import { ref } from 'vue'

import type { MetricPlaceholderDescriptor } from '@/api/contracts'
import type { BaseDisplay } from '@/models/devices/display/display'
import type { DisplayCanvasInteraction } from '@/models/devices/display/canvas/interaction'
import { resolveDisplayInteractionWidgets } from '@/models/devices/display/canvas/interaction'
import type { DisplayWidget } from '@/models/devices/display/layout'
import { resolveMetricPlaceholderText } from '@/models/metrics/placeholders'
import type { RasterImageFormat } from '@/raster/raster-image-types'

import DisplayWidgetPreview from './DisplayWidgetPreview.vue'

type CanvasInteraction = DisplayCanvasInteraction & { pointerId: number }

const props = defineProps<{
  widgets: DisplayWidget[]
  deviceWidth: number
  deviceHeight: number
  selectedWidgetId: string | null
  zoom: number
  display: BaseDisplay<RasterImageFormat>
  metricCatalog: readonly MetricPlaceholderDescriptor[]
}>()

const emit = defineEmits<{
  'select-widget': [widgetId: string]
  'update-widgets': [widgets: DisplayWidget[]]
  'interaction-change': [state: { widgetId: string | null; mode: 'drag' | 'resize' | null }]
}>()

const activeInteraction = ref<CanvasInteraction | null>(null)

function widgetPreviewText(widget: DisplayWidget): string {
  return widget.type === 'text' ? resolveMetricPlaceholderText(widget.text, props.metricCatalog) : ''
}

function isBitmapResizeActive(widgetId: string, widgetType: DisplayWidget['type']): boolean {
  return widgetType === 'bitmap' && activeInteraction.value?.mode === 'resize' && activeInteraction.value.widgetId === widgetId
}

function startDrag(event: PointerEvent, widget: DisplayWidget): void {
  if (event.button !== 0) return
  startInteraction(event, widget, 'drag')
}

function startResize(event: PointerEvent, widget: DisplayWidget): void {
  if (event.button !== 0) return
  startInteraction(event, widget, 'resize')
}

function startInteraction(event: PointerEvent, widget: DisplayWidget, mode: CanvasInteraction['mode']): void {
  emit('select-widget', widget.id)
  activeInteraction.value = {
    pointerId: event.pointerId,
    mode,
    widgetId: widget.id,
    keepAspectRatio: widget.type === 'bitmap' && 'keepAspectRatio' in widget ? widget.keepAspectRatio : false,
    startClientX: event.clientX,
    startClientY: event.clientY,
    startX: widget.x,
    startY: widget.y,
    startWidth: widget.width,
    startHeight: widget.height,
    startAspectRatio: widget.height > 0 ? widget.width / widget.height : 1,
  }
  const target = event.currentTarget
  if (target instanceof Element) {
    target.setPointerCapture(event.pointerId)
    target.addEventListener('pointermove', updateInteraction)
    target.addEventListener('pointerup', finishInteraction, { once: true })
    target.addEventListener('pointercancel', finishInteraction, { once: true })
  }
  emit('interaction-change', { widgetId: widget.id, mode })
  event.preventDefault()
}

function updateInteraction(event: Event): void {
  const pointerEvent = event as PointerEvent
  const interaction = activeInteraction.value
  if (interaction === null || interaction.pointerId !== pointerEvent.pointerId) return
  emit('update-widgets', resolveDisplayInteractionWidgets(
    props.widgets,
    interaction,
    pointerEvent.clientX,
    pointerEvent.clientY,
    props.zoom,
    props.deviceWidth,
    props.deviceHeight,
  ))
}

function finishInteraction(event: Event): void {
  const pointerEvent = event as PointerEvent
  const target = event.currentTarget
  if (target instanceof Element) {
    target.releasePointerCapture(pointerEvent.pointerId)
    target.removeEventListener('pointermove', updateInteraction)
    target.removeEventListener('pointerup', finishInteraction)
    target.removeEventListener('pointercancel', finishInteraction)
  }
  activeInteraction.value = null
  emit('interaction-change', { widgetId: null, mode: null })
}
</script>
