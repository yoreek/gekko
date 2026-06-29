<template>
  <section class="tft-layout-preview">
    <header class="tft-layout-preview__header">
      <div class="text-subtitle-2">{{ t('device.dialog.st7735Display.previewTitle') }}</div>
      <div class="text-caption text-medium-emphasis">
        {{ layout.pages.find(page => page.id === layout.activePageId)?.name ?? layout.activePageId }}
      </div>
    </header>

    <div class="tft-layout-preview__surface">
      <div class="tft-layout-preview__canvas" :style="canvasStyle">
        <div
          v-for="widget in activePageWidgets"
          :key="widget.id"
          class="tft-layout-preview__widget"
          :style="widgetStyle(widget)"
        >
          <St7735WidgetPreview
            :widget="widget"
            :display="display"
            :display-scale="previewScale"
            :freeze-render="isBitmapRenderFrozen && widget.type === 'bitmap'"
            :preview-text="widgetPreviewText(widget)"
          />
        </div>
      </div>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import St7735WidgetPreview from '@/components/devices/display/st7735/St7735WidgetPreview.vue'
import { resolveSsd1306CanvasStyle, resolveSsd1306WidgetFrameStyle } from '@/components/devices/display/ssd1306/ssd1306-layout-math'
import type { MetricPlaceholderDescriptor } from '@/api/contracts'
import type { BaseDisplay } from '@/models/devices/display/display'
import type { Ssd1306LayoutDraft, Ssd1306Widget } from '@/models/devices/ssd1306/layout'
import { resolveMetricPlaceholderText } from '@/models/metrics/placeholders'

const props = defineProps<{
  layout: Ssd1306LayoutDraft
  display: BaseDisplay<'rgb565'>
  deviceWidth?: number
  deviceHeight?: number
  previewScale?: number
  bitmapRenderFrozen?: boolean
  metricCatalog?: readonly MetricPlaceholderDescriptor[]
}>()

const { t } = useI18n()

const previewScale = computed(() => props.previewScale ?? 1.75)
const isBitmapRenderFrozen = computed(() => Boolean(props.bitmapRenderFrozen))
const metricCatalog = computed(() => props.metricCatalog ?? [])
const activePageWidgets = computed(() => props.layout.pages.find(page => page.id === props.layout.activePageId)?.widgets ?? [])
const canvasStyle = computed(() => resolveSsd1306CanvasStyle(Math.max(1, props.deviceWidth ?? 128), Math.max(1, props.deviceHeight ?? 160), previewScale.value))

function widgetPreviewText(widget: Ssd1306Widget): string {
  return widget.type === 'text' ? resolveMetricPlaceholderText(widget.text, metricCatalog.value) : ''
}

function widgetStyle(widget: Ssd1306Widget): Record<string, string> {
  return resolveSsd1306WidgetFrameStyle(widget, previewScale.value)
}
</script>

<style scoped>
.tft-layout-preview {
  display: grid;
  gap: 8px;
  justify-items: center;
}

.tft-layout-preview__header {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 12px;
}

.tft-layout-preview__surface {
  justify-self: center;
  max-width: 100%;
  overflow: auto;
  padding-top: 12px;
  border: 0;
  border-radius: 0;
  background: transparent;
}

.tft-layout-preview__canvas {
  position: relative;
  overflow: hidden;
  border: 1px solid rgba(var(--v-theme-primary), 0.14);
  border-radius: 0;
  box-sizing: content-box;
  background: rgb(0 0 0);
  box-shadow: inset 0 0 0 1px rgba(var(--v-theme-on-surface), 0.04);
}

.tft-layout-preview__widget {
  position: absolute;
  overflow: hidden;
  color: rgb(var(--v-theme-on-surface));
}
</style>
