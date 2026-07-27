<template>
  <svg
    :width="outerWidth * effectiveZoom"
    :height="outerHeight * effectiveZoom"
    :viewBox="`0 0 ${outerWidth} ${outerHeight}`"
    role="application"
  >
    <g :transform="contentTransform">
      <rect x="0" y="0" :width="deviceWidth" :height="deviceHeight" :fill="backgroundColor" />

      <g v-for="widget in widgets" :key="widget.id" :transform="`translate(${widget.x}, ${widget.y})`">
        <g
          role="button"
          tabindex="0"
          @pointerdown="startDrag($event, widget)"
          @click="!realPosition && $emit('select-widget', widget.id)"
          @keydown.enter.prevent="!realPosition && $emit('select-widget', widget.id)"
          @keydown.space.prevent="!realPosition && $emit('select-widget', widget.id)"
        >
          <rect :width="Math.max(1, widget.width)" :height="Math.max(1, widget.height)" fill="transparent" />
          <DisplayWidgetPreview
            :widget="widget"
            :display="display"
            :background-color="backgroundColor"
            :preview-text="widgetPreviewText(widget)"
            :freeze-render="isBitmapResizeActive(widget.id, widget.type)"
          />
        </g>
        <rect
          v-if="widget.id === selectedWidgetId && !realPosition"
          :width="Math.max(1, widget.width)"
          :height="Math.max(1, widget.height)"
          fill="none"
          stroke="rgb(var(--v-theme-primary))"
          stroke-width="1"
          vector-effect="non-scaling-stroke"
          pointer-events="none"
        />
        <rect
          v-if="widget.id === selectedWidgetId && !realPosition"
          :x="Math.max(0, widget.width - selectionHandleSize)"
          :y="Math.max(0, widget.height - selectionHandleSize)"
          :width="selectionHandleSize"
          :height="selectionHandleSize"
          fill="rgb(var(--v-theme-primary))"
          @pointerdown.stop="startResize($event, widget)"
        />
      </g>
    </g>
  </svg>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'

import type { MetricPlaceholderDescriptor } from '@/api/contracts'
import type { BaseDisplay } from '@/models/devices/display/display'
import type { DisplayCanvasInteraction } from '@/models/devices/display/canvas/interaction'
import { resolveDisplayInteractionWidgets } from '@/models/devices/display/canvas/interaction'
import type { DisplayWidget } from '@/models/devices/display/layout'
import { resolveMetricPlaceholderText } from '@/models/metrics/placeholders'
import type { RasterImageFormat } from '@/raster/raster-image-types'

import DisplayWidgetPreview from './DisplayWidgetPreview.vue'

type CanvasInteraction = DisplayCanvasInteraction & { pointerId: number }

const props = withDefaults(
  defineProps<{
    widgets: DisplayWidget[]
    deviceWidth: number
    deviceHeight: number
    selectedWidgetId: string | null
    zoom: number
    display: BaseDisplay<RasterImageFormat>
    backgroundColor: string
    metricCatalog: readonly MetricPlaceholderDescriptor[]
    // When true, render widgets rotated into the panel's native (rotation=0) frame instead of the
    // normalized/upright working frame -- a read-only preview of how the panel will physically show
    // this layout. 0° and 180° are otherwise visually identical in the working frame.
    realPosition?: boolean
    rotation?: number
    nativeWidth?: number
    nativeHeight?: number
  }>(),
  { realPosition: false, rotation: 0, nativeWidth: undefined, nativeHeight: undefined },
)

const emit = defineEmits<{
  'select-widget': [widgetId: string]
  'update-widgets': [widgets: DisplayWidget[]]
  'interaction-change': [state: { widgetId: string | null; mode: 'drag' | 'resize' | null }]
}>()

const activeInteraction = ref<CanvasInteraction | null>(null)

const effectiveZoom = computed(() => Math.max(1, props.zoom) * props.display.displayCapabilities.canvasUnitSize.width)
const selectionHandleSize = computed(() => 6 / effectiveZoom.value)

const outerWidth = computed(() => (props.realPosition ? props.nativeWidth ?? props.deviceWidth : props.deviceWidth))
const outerHeight = computed(() => (props.realPosition ? props.nativeHeight ?? props.deviceHeight : props.deviceHeight))

const contentTransform = computed(() => {
  if (!props.realPosition) return undefined
  const angle = ((props.rotation % 4) + 4) % 4 * 90
  const nativeWidth = props.nativeWidth ?? props.deviceWidth
  const nativeHeight = props.nativeHeight ?? props.deviceHeight
  return `translate(${nativeWidth / 2}, ${nativeHeight / 2}) rotate(${angle}) translate(${-props.deviceWidth / 2}, ${-props.deviceHeight / 2})`
})

function widgetPreviewText(widget: DisplayWidget): string {
  return widget.type === 'text' || widget.type === 'character' || widget.type === 'digital'
    ? resolveMetricPlaceholderText(widget.text, props.metricCatalog)
    : ''
}

function isBitmapResizeActive(widgetId: string, widgetType: DisplayWidget['type']): boolean {
  return widgetType === 'bitmap' && activeInteraction.value?.mode === 'resize' && activeInteraction.value.widgetId === widgetId
}

function startDrag(event: PointerEvent, widget: DisplayWidget): void {
  if (props.realPosition || event.button !== 0) return
  startInteraction(event, widget, 'drag')
}

function startResize(event: PointerEvent, widget: DisplayWidget): void {
  if (props.realPosition || event.button !== 0) return
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
  const resolvedWidgets = resolveDisplayInteractionWidgets(
    props.widgets,
    interaction,
    pointerEvent.clientX,
    pointerEvent.clientY,
    effectiveZoom.value,
    props.deviceWidth,
    props.deviceHeight,
  )
  emit('update-widgets', resolvedWidgets.map((widget) => {
    if (widget.type !== 'bitmap') return widget
    const current = props.widgets.find(candidate => candidate.id === widget.id)
    if (current?.type !== 'bitmap' || (current.width === widget.width && current.height === widget.height)) return widget
    return props.display.supportsBitmapSize(widget.width, widget.height)
      ? props.display.resizeWidget(current, { width: widget.width, height: widget.height })
      : current
  }))
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
