<template>
  <GridLayout
    v-model:layout="gridLayout"
    class="oled-canvas"
    :col-num="deviceWidth"
    :is-draggable="true"
    :is-resizable="true"
    :is-bounded="true"
    :margin="[0, 0]"
    :prevent-collision="false"
    :row-height="zoom"
    :style="canvasStyle"
    :use-css-transforms="true"
    :vertical-compact="false"
  >
    <GridItem
      v-for="item in widgets"
      :key="item.id"
      :i="item.id"
      :x="item.x"
      :y="item.y"
      :w="item.width"
      :h="item.height"
      class="oled-canvas__item"
    >
      <div
        class="oled-canvas__widget"
        :class="{ 'oled-canvas__widget--selected': item.id === selectedWidgetId }"
        role="button"
        tabindex="0"
        @click="$emit('select-widget', item.id)"
        @keydown.enter.prevent="$emit('select-widget', item.id)"
        @keydown.space.prevent="$emit('select-widget', item.id)"
      >
        <OledDisplayWidgetPreview :widget="item" :display-scale="zoom" />
      </div>
    </GridItem>
  </GridLayout>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { GridItem, GridLayout } from 'vue-grid-layout-v3'

import OledDisplayWidgetPreview from '@/components/devices/oled-display/OledDisplayWidgetPreview.vue'
import { resolveOledDisplayCanvasStyle } from '@/components/devices/oled-display/oled-display-layout-math'
import type { OledDisplayWidget } from '@/models/devices/oled-display-layout'

interface GridLayoutItem {
  i: string
  x: number
  y: number
  w: number
  h: number
}

const props = defineProps<{
  widgets: OledDisplayWidget[]
  deviceWidth: number
  deviceHeight: number
  selectedWidgetId: string | null
  zoom: number
}>()

const emit = defineEmits<{
  'select-widget': [widgetId: string]
  'update-widgets': [widgets: OledDisplayWidget[]]
}>()

const gridLayout = computed<GridLayoutItem[]>({
  get: () => props.widgets.map(widget => ({
    i: widget.id,
    x: widget.x,
    y: widget.y,
    w: widget.width,
    h: widget.height,
  })),
  set: layoutUpdated,
})

const canvasStyle = computed(() => resolveOledDisplayCanvasStyle(props.deviceWidth, props.deviceHeight, props.zoom))

function layoutUpdated(layout: GridLayoutItem[]): void {
  const nextWidgets = layout.map(item => {
    const current = props.widgets.find(widget => widget.id === item.i)
    if (current !== undefined) {
      return {
        ...current,
        x: item.x,
        y: item.y,
        width: item.w,
        height: item.h,
      } as OledDisplayWidget
    }
    return {
      id: item.i,
      type: 'text',
      x: item.x,
      y: item.y,
      width: item.w,
      height: item.h,
      bindingKind: 'unbound',
      sourceDeviceId: 0,
      metricId: 0,
      text: 'ABC',
      fontSize: 1,
      strokeWidth: 1,
      styleFlags: { filled: false, inverted: false, wrap: false },
    } as OledDisplayWidget
  })
  emit('update-widgets', nextWidgets)
}
</script>

<style scoped>
.oled-canvas {
  position: relative;
  overflow-x: hidden;
  overflow-y: auto;
  padding: 0;
  border: 1px solid rgba(var(--v-theme-on-surface), 0.16);
  border-radius: 0;
  background:
    linear-gradient(90deg, rgba(var(--v-theme-on-surface), 0.1) 1px, transparent 1px),
    linear-gradient(rgba(var(--v-theme-on-surface), 0.1) 1px, transparent 1px),
    linear-gradient(rgba(var(--v-theme-on-surface), 0.02), rgba(var(--v-theme-on-surface), 0.02)),
    rgb(var(--v-theme-surface));
  box-shadow: inset 0 0 0 1px rgba(var(--v-theme-on-surface), 0.06);
}

.oled-canvas__item {
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
  cursor: pointer;
}
</style>
