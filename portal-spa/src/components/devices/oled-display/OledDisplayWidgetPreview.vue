<template>
  <div class="oled-widget-preview" :class="[`oled-widget-preview--${widget.type}`]">
    <canvas
      v-if="widget.type === 'text'"
      ref="textCanvasRef"
      class="oled-widget-preview__text"
      :aria-label="previewLabel"
    >
    </canvas>
    <v-icon v-else-if="widget.type === 'icon'" :icon="iconName" :size="iconSize" />
    <div v-else-if="widget.type === 'rect'" class="oled-widget-preview__shape oled-widget-preview__shape--rect" :class="{ 'oled-widget-preview__shape--filled': widget.styleFlags.filled }" />
    <div v-else-if="widget.type === 'line'" class="oled-widget-preview__shape oled-widget-preview__shape--line" />
    <div v-else class="oled-widget-preview__shape oled-widget-preview__shape--circle" :class="{ 'oled-widget-preview__shape--filled': widget.styleFlags.filled }" />
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import { drawClassicFontText } from '@/components/devices/oled-display/classic-font'
import { resolveOledDisplayIconSize, resolveOledDisplayTextRenderScale, resolveOledDisplayWidgetBitmapSize } from '@/components/devices/oled-display/oled-display-layout-math'
import type { OledDisplayWidget } from '@/models/devices/oled-display-layout'

const props = defineProps<{
  widget: OledDisplayWidget
  displayScale?: number
}>()

const { t } = useI18n()

const textCanvasRef = ref<HTMLCanvasElement | null>(null)
let resizeObserver: ResizeObserver | null = null
let drawFrame = 0

const iconName = computed(() => (props.widget.text.trim().length > 0 ? props.widget.text : 'oled-icon'))
const displayScale = computed(() => Math.max(1, props.displayScale ?? 1))
const iconSize = computed(() => resolveOledDisplayIconSize(displayScale.value))
const previewLabel = computed(() => props.widget.text.trim().length > 0 ? props.widget.text : t('device.dialog.oledDisplay.widgetEmpty'))
const renderScale = computed(() => resolveOledDisplayTextRenderScale(props.widget.fontSize, 1))
const oledOnColor = 'rgb(255 255 255)'
const oledOffColor = 'rgb(0 0 0)'

function scheduleDraw(): void {
  if (drawFrame !== 0) {
    return
  }
  drawFrame = window.requestAnimationFrame(() => {
    drawFrame = 0
    drawTextCanvas()
  })
}

function drawTextCanvas(): void {
  const canvas = textCanvasRef.value
  if (canvas === null) {
    return
  }

  const canvasSize = resolveOledDisplayWidgetBitmapSize(props.widget.width, props.widget.height)

  if (canvas.width !== canvasSize.bitmapWidth) {
    canvas.width = canvasSize.bitmapWidth
  }
  if (canvas.height !== canvasSize.bitmapHeight) {
    canvas.height = canvasSize.bitmapHeight
  }

  const context = canvas.getContext('2d')
  if (context === null) {
    return
  }

  context.setTransform(1, 0, 0, 1, 0, 0)
  const inverted = props.widget.styleFlags.inverted
  drawClassicFontText(context, previewLabel.value, {
    scale: renderScale.value,
    wrap: props.widget.styleFlags.wrap,
    maxWidth: canvasSize.cssWidth,
    maxHeight: canvasSize.cssHeight,
    color: inverted ? oledOffColor : oledOnColor,
    backgroundColor: inverted ? oledOnColor : oledOffColor,
  })
}

onMounted(() => {
  const canvas = textCanvasRef.value
  if (canvas === null) {
    return
  }

  resizeObserver = new ResizeObserver(() => {
    scheduleDraw()
  })
  resizeObserver.observe(canvas.parentElement ?? canvas)
  scheduleDraw()
})

onBeforeUnmount(() => {
  resizeObserver?.disconnect()
  resizeObserver = null
  if (drawFrame !== 0) {
    window.cancelAnimationFrame(drawFrame)
    drawFrame = 0
  }
})

watch(
  () => [
    previewLabel.value,
    renderScale.value,
    props.widget.width,
    props.widget.height,
    props.widget.styleFlags.wrap,
    props.widget.styleFlags.inverted,
  ],
  async () => {
    await nextTick()
    drawTextCanvas()
  },
  { immediate: true, flush: 'post' },
)
</script>

<style scoped>
.oled-widget-preview {
  display: grid;
  align-items: start;
  justify-items: start;
  width: 100%;
  height: 100%;
  overflow: hidden;
  background: rgb(0 0 0);
  color: rgb(var(--v-theme-on-surface));
}

.oled-widget-preview__text {
  display: block;
  width: 100%;
  height: 100%;
  max-width: 100%;
  max-height: 100%;
  image-rendering: pixelated;
}

.oled-widget-preview__shape {
  border-color: currentColor;
}

.oled-widget-preview__shape--rect {
  width: 100%;
  height: 100%;
  border: 1px solid currentColor;
}

.oled-widget-preview__shape--rect.oled-widget-preview__shape--filled {
  background: currentColor;
}

.oled-widget-preview__shape--line {
  width: 100%;
  border-top: 2px solid currentColor;
}

.oled-widget-preview__shape--circle {
  width: 100%;
  height: 100%;
  border: 1px solid currentColor;
  border-radius: 50%;
}

.oled-widget-preview__shape--circle.oled-widget-preview__shape--filled {
  background: currentColor;
}
</style>
