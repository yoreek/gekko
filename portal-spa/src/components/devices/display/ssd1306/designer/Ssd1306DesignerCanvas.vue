<template>
  <div
    class="oled-canvas"
    :style="canvasStyle"
    role="application"
  >
    <div
      v-for="item in widgets"
      :key="item.id"
      class="oled-canvas__item"
      :style="widgetFrameStyle(item)"
    >
      <div
        class="oled-canvas__widget"
        :class="{ 'oled-canvas__widget--selected': item.id === selectedWidgetId }"
        role="button"
        tabindex="0"
        @pointerdown="startDrag($event, item)"
        @click="$emit('select-widget', item.id)"
        @keydown.enter.prevent="$emit('select-widget', item.id)"
        @keydown.space.prevent="$emit('select-widget', item.id)"
      >
        <Ssd1306WidgetPreview
          :widget="item"
          :display="display"
          :display-scale="zoom"
          :freeze-render="isBitmapResizeActive(item.id, item.type)"
        />
        <span
          class="oled-canvas__resize-handle"
          aria-hidden="true"
          @pointerdown.stop="startResize($event, item)"
        />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'

import Ssd1306WidgetPreview from '@/components/devices/display/ssd1306/Ssd1306WidgetPreview.vue'
import { resolveSsd1306InteractionWidgets, type Ssd1306CanvasInteraction } from '@/components/devices/display/ssd1306/ssd1306-editor-interaction'
import { resolveSsd1306CanvasStyle, resolveSsd1306WidgetFrameStyle } from '@/components/devices/display/ssd1306/ssd1306-layout-math'
import type { BaseDisplay } from '@/models/devices/display/display'
import type { Ssd1306Widget } from '@/models/devices/ssd1306/layout'

type CanvasInteraction = Ssd1306CanvasInteraction & {
  pointerId: number
}

const props = defineProps<{
  widgets: Ssd1306Widget[]
  deviceWidth: number
  deviceHeight: number
  selectedWidgetId: string | null
  zoom: number
  display: BaseDisplay<'mono1'>
}>()

const emit = defineEmits<{
  'select-widget': [widgetId: string]
  'update-widgets': [widgets: Ssd1306Widget[]]
  'interaction-change': [state: { widgetId: string | null; mode: 'drag' | 'resize' | null }]
}>()

const activeInteraction = ref<CanvasInteraction | null>(null)

const canvasStyle = computed(() => resolveSsd1306CanvasStyle(props.deviceWidth, props.deviceHeight, props.zoom))

function widgetFrameStyle(widget: Ssd1306Widget): Record<string, string> {
  return resolveSsd1306WidgetFrameStyle(widget, props.zoom)
}

function isBitmapResizeActive(widgetId: string, widgetType: Ssd1306Widget['type']): boolean {
  return widgetType === 'bitmap' && activeInteraction.value?.mode === 'resize' && activeInteraction.value.widgetId === widgetId
}

function startDrag(event: PointerEvent, widget: Ssd1306Widget): void {
  if (event.button !== 0) {
    return
  }
  startInteraction(event, widget, 'drag')
}

function startResize(event: PointerEvent, widget: Ssd1306Widget): void {
  if (event.button !== 0) {
    return
  }
  startInteraction(event, widget, 'resize')
}

function startInteraction(event: PointerEvent, widget: Ssd1306Widget, mode: CanvasInteraction['mode']): void {
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
  if (target instanceof HTMLElement) {
    target.setPointerCapture(event.pointerId)
    target.addEventListener('pointermove', updateInteraction)
    target.addEventListener('pointerup', finishInteraction, { once: true })
    target.addEventListener('pointercancel', finishInteraction, { once: true })
  }
  emit('interaction-change', { widgetId: widget.id, mode })
  event.preventDefault()
}

function updateInteraction(event: PointerEvent): void {
  const interaction = activeInteraction.value
  if (interaction === null || interaction.pointerId !== event.pointerId) {
    return
  }
  emit('update-widgets', resolveSsd1306InteractionWidgets(props.widgets, interaction, event.clientX, event.clientY, props.zoom, props.deviceWidth, props.deviceHeight))
}

function finishInteraction(event: PointerEvent): void {
  const target = event.currentTarget
  if (target instanceof HTMLElement) {
    target.releasePointerCapture(event.pointerId)
    target.removeEventListener('pointermove', updateInteraction)
    target.removeEventListener('pointerup', finishInteraction)
    target.removeEventListener('pointercancel', finishInteraction)
  }
  activeInteraction.value = null
  emit('interaction-change', { widgetId: null, mode: null })
}

</script>

<style scoped>
.oled-canvas {
  position: relative;
  overflow-x: hidden;
  overflow-y: hidden;
  padding: 0;
  border: 1px solid rgba(var(--v-theme-on-surface), 0.16);
  border-radius: 0;
  background:
    linear-gradient(90deg, rgba(255, 255, 255, 0.12) 1px, transparent 1px),
    linear-gradient(rgba(255, 255, 255, 0.12) 1px, transparent 1px),
    rgb(0 0 0);
  box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.08);
}

.oled-canvas__item {
  position: absolute;
  overflow: visible;
}

.oled-canvas__widget--selected::after {
  content: '';
  position: absolute;
  inset: 0;
  border: 2px solid rgb(var(--v-theme-primary));
  pointer-events: none;
  box-sizing: border-box;
}

.oled-canvas__widget {
  position: relative;
  display: grid;
  place-items: center;
  width: 100%;
  height: 100%;
  padding: 0;
  border: 0;
  box-sizing: content-box;
  background: transparent;
  color: rgb(var(--v-theme-on-surface));
  cursor: move;
  touch-action: none;
}

.oled-canvas__resize-handle {
  position: absolute;
  right: 0;
  bottom: 0;
  width: 8px;
  height: 8px;
  border-right: 2px solid rgb(var(--v-theme-primary));
  border-bottom: 2px solid rgb(var(--v-theme-primary));
  opacity: 0;
  cursor: nwse-resize;
  pointer-events: auto;
}

.oled-canvas__widget--selected .oled-canvas__resize-handle {
  opacity: 1;
}
</style>
