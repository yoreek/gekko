<template>
  <section class="oled-layout-preview">
    <header class="oled-layout-preview__header">
      <div class="text-subtitle-2">{{ t('device.dialog.oledDisplay.previewTitle') }}</div>
      <div class="text-caption text-medium-emphasis">
        {{ layout.pages.find(page => page.id === layout.activePageId)?.name ?? layout.activePageId }}
      </div>
    </header>

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
          <OledDisplayWidgetPreview :widget="widget" :display-scale="previewScale" />
        </div>
      </div>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import OledDisplayWidgetPreview from '@/components/devices/oled-display/OledDisplayWidgetPreview.vue'
import { resolveOledDisplayCanvasStyle, resolveOledDisplayWidgetFrameStyle } from '@/components/devices/oled-display/oled-display-layout-math'
import type { OledDisplayLayoutDraft, OledDisplayWidget } from '@/models/devices/oled-display-layout'

const props = defineProps<{
  layout: OledDisplayLayoutDraft
  deviceWidth?: number
  deviceHeight?: number
  previewScale?: number
}>()

const { t } = useI18n()

const previewScale = computed(() => props.previewScale ?? 1.75)
const activePageWidgets = computed(() => props.layout.pages.find(page => page.id === props.layout.activePageId)?.widgets ?? [])
const canvasStyle = computed(() => resolveOledDisplayCanvasStyle(Math.max(1, props.deviceWidth ?? 128), Math.max(1, props.deviceHeight ?? 64), previewScale.value))

function widgetStyle(widget: OledDisplayWidget): Record<string, string> {
  return resolveOledDisplayWidgetFrameStyle(widget, previewScale.value)
}
</script>

<style scoped>
.oled-layout-preview {
  display: grid;
  gap: 8px;
}

.oled-layout-preview__header {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 12px;
}

.oled-layout-preview__surface {
  justify-self: start;
  max-width: 100%;
  overflow: auto;
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
