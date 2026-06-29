<template>
  <section class="oled-layout-preview">
    <div class="oled-layout-preview__surface">
      <div
        class="oled-layout-preview__canvas"
        :style="canvasStyle"
      >
        <div
          v-for="widget in activePageWidgets"
          :key="widget.id"
          class="oled-layout-preview__widget"
          :style="widgetStyle(widget)"
        >
          <Ssd1306WidgetPreview
            :widget="widget"
            :display="display"
            :display-scale="previewScale"
            :freeze-render="isBitmapRenderFrozen && widget.type === 'bitmap'"
          />
        </div>
      </div>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'

import Ssd1306WidgetPreview from '@/components/devices/display/ssd1306/Ssd1306WidgetPreview.vue'
import { resolveSsd1306CanvasStyle, resolveSsd1306WidgetFrameStyle } from '@/components/devices/display/ssd1306/ssd1306-layout-math'
import type { BaseDisplay } from '@/models/devices/display/display'
import type { Ssd1306LayoutDraft, Ssd1306Widget } from '@/models/devices/ssd1306/layout'

const props = defineProps<{
  layout: Ssd1306LayoutDraft
  display: BaseDisplay<'mono1'>
  deviceWidth?: number
  deviceHeight?: number
  previewScale?: number
  bitmapRenderFrozen?: boolean
}>()

const previewScale = computed(() => props.previewScale ?? 1.75)
const isBitmapRenderFrozen = computed(() => Boolean(props.bitmapRenderFrozen))
const activePageWidgets = computed(() => props.layout.pages.find(page => page.id === props.layout.activePageId)?.widgets ?? [])
const canvasStyle = computed(() => resolveSsd1306CanvasStyle(Math.max(1, props.deviceWidth ?? 128), Math.max(1, props.deviceHeight ?? 64), previewScale.value))

function widgetStyle(widget: Ssd1306Widget): Record<string, string> {
  return resolveSsd1306WidgetFrameStyle(widget, previewScale.value)
}
</script>

<style scoped>
.oled-layout-preview {
  display: grid;
  gap: 8px;
  justify-items: center;
}

.oled-layout-preview__surface {
  justify-self: center;
  max-width: 100%;
  overflow: auto;
  padding-top: 12px;
  border: 0;
  border-radius: 0;
  background: transparent;
}

.oled-layout-preview__canvas {
  position: relative;
  overflow: hidden;
  border: 1px solid rgba(var(--v-theme-primary), 0.14);
  border-radius: 0;
  box-sizing: content-box;
  background: rgb(0 0 0);
  box-shadow: inset 0 0 0 1px rgba(var(--v-theme-on-surface), 0.04);
}

.oled-layout-preview__widget {
  position: absolute;
  overflow: hidden;
  color: rgb(var(--v-theme-on-surface));
}
</style>
