<template>
  <div class="tft-widget-preview" :class="[`tft-widget-preview--${widget.type}`]">
    <canvas
      v-if="widget.type === 'text'"
      ref="textCanvasRef"
      class="tft-widget-preview__text"
      :aria-label="previewLabel"
    />
    <v-icon v-else-if="widget.type === 'icon'" :icon="iconName" :size="iconSize" />
    <canvas
      v-else-if="widget.type === 'bitmap'"
      ref="bitmapCanvasRef"
      class="tft-widget-preview__bitmap"
      :class="{ 'tft-widget-preview__bitmap--frozen': freezeRender }"
      aria-hidden="true"
    />
    <canvas
      v-else-if="isShapeWidget"
      ref="shapeCanvasRef"
      class="tft-widget-preview__shape"
      aria-hidden="true"
    />
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import { drawClassicFontText } from '@/components/devices/display/ssd1306/classic-font'
import { resolveSsd1306IconSize, resolveSsd1306TextRenderScale, resolveSsd1306WidgetBitmapSize } from '@/components/devices/display/ssd1306/ssd1306-layout-math'
import type { BaseDisplay } from '@/models/devices/display/display'
import type { DisplayBitmapWidget } from '@/models/devices/display/layout'
import type { DisplayWidget } from '@/models/devices/display/layout'

const props = defineProps<{
  widget: DisplayWidget
  display: BaseDisplay<'rgb565'>
  displayScale?: number
  freezeRender?: boolean
}>()

const { t } = useI18n()

const textCanvasRef = ref<HTMLCanvasElement | null>(null)
const bitmapCanvasRef = ref<HTMLCanvasElement | null>(null)
const shapeCanvasRef = ref<HTMLCanvasElement | null>(null)
let resizeObserver: ResizeObserver | null = null
let drawFrame = 0

const iconName = computed(() => (props.widget.text.trim().length > 0 ? props.widget.text : 'oled-icon'))
const displayScale = computed(() => Math.max(1, props.displayScale ?? 1))
const iconSize = computed(() => resolveSsd1306IconSize(displayScale.value))
const isShapeWidget = computed(() => props.widget.type === 'rect' || props.widget.type === 'line' || props.widget.type === 'circle' || props.widget.type === 'ellipse')
const isBitmapWidget = computed(() => props.widget.type === 'bitmap')
const previewLabel = computed(() => props.widget.text.trim().length > 0 ? props.widget.text : t('device.dialog.ssd1306Display.widgetEmpty'))
const renderScale = computed(() => resolveSsd1306TextRenderScale(props.widget.fontSize, 1))
const isRenderFrozen = computed(() => Boolean(props.freezeRender))
const onColor = 'rgb(255 255 255)'
const offColor = 'rgb(0 0 0)'

function scheduleDraw(): void {
  if (drawFrame !== 0) {
    return
  }
  drawFrame = window.requestAnimationFrame(() => {
    drawFrame = 0
    drawTextCanvas()
  })
}

function drawBitmapCanvas(): void {
  if (isRenderFrozen.value) {
    return
  }
  const canvas = bitmapCanvasRef.value
  if (canvas === null || !isBitmapWidget.value) {
    return
  }

  const canvasSize = resolveSsd1306WidgetBitmapSize(props.widget.width, props.widget.height)
  if (canvas.width !== canvasSize.bitmapWidth) {
    canvas.width = canvasSize.bitmapWidth
  }
  if (canvas.height !== canvasSize.bitmapHeight) {
    canvas.height = canvasSize.bitmapHeight
  }

  try {
    props.display.renderWidget(props.widget as DisplayBitmapWidget, canvas)
  } catch {
    return
  }
}

function drawShapeCanvas(): void {
  const canvas = shapeCanvasRef.value
  if (canvas === null || !isShapeWidget.value) {
    return
  }

  const canvasSize = resolveSsd1306WidgetBitmapSize(props.widget.width, props.widget.height)
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

  const inverted = props.widget.styleFlags.inverted
  const color = inverted ? offColor : onColor
  const width = canvasSize.cssWidth
  const height = canvasSize.cssHeight
  const strokeWidth = Math.max(1, Math.round(props.widget.strokeWidth))

  context.setTransform(1, 0, 0, 1, 0, 0)
  context.imageSmoothingEnabled = false
  context.clearRect(0, 0, width, height)
  context.fillStyle = color
  context.strokeStyle = color
  context.lineWidth = strokeWidth

  if (props.widget.type === 'rect') {
    if (props.widget.styleFlags.filled) {
      context.fillRect(0, 0, width, height)
    } else {
      context.strokeRect(strokeWidth / 2, strokeWidth / 2, Math.max(0, width - strokeWidth), Math.max(0, height - strokeWidth))
    }
    return
  }

  if (props.widget.type === 'line') {
    context.beginPath()
    context.moveTo(0, Math.max(0, Math.floor(height / 2)))
    context.lineTo(width, Math.max(0, Math.floor(height / 2)))
    context.stroke()
    return
  }

  const centerX = width / 2
  const centerY = height / 2
  const radiusX = Math.max(0.5, (width - strokeWidth) / 2)
  const radiusY = Math.max(0.5, (height - strokeWidth) / 2)
  context.beginPath()
  context.ellipse(centerX, centerY, radiusX, radiusY, 0, 0, Math.PI * 2)
  if (props.widget.styleFlags.filled) {
    context.fill()
  } else {
    context.stroke()
  }
}

function drawTextCanvas(): void {
  const canvas = textCanvasRef.value
  if (canvas === null) {
    return
  }

  const canvasSize = resolveSsd1306WidgetBitmapSize(props.widget.width, props.widget.height)

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
    color: inverted ? offColor : onColor,
    backgroundColor: inverted ? onColor : offColor,
  })
}

onMounted(() => {
  const canvas = textCanvasRef.value ?? shapeCanvasRef.value ?? bitmapCanvasRef.value
  if (canvas === null) {
    return
  }

  resizeObserver = new ResizeObserver(() => {
    scheduleDraw()
  })
  resizeObserver.observe(canvas.parentElement ?? canvas)
  scheduleDraw()
  drawShapeCanvas()
  drawBitmapCanvas()
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

watch(
  () => [
    props.widget.type,
    props.widget.width,
    props.widget.height,
    props.widget.strokeWidth,
    props.widget.styleFlags.filled,
    props.widget.styleFlags.inverted,
  ],
  async () => {
    await nextTick()
    drawShapeCanvas()
  },
  { immediate: true, flush: 'post' },
)

watch(
  () => [
    props.widget.type,
    props.widget.width,
    props.widget.height,
    props.widget.type === 'bitmap' ? props.widget.bitmapData : '',
    isRenderFrozen.value,
  ],
  async () => {
    await nextTick()
    drawBitmapCanvas()
  },
  { immediate: true, flush: 'post' },
)
</script>

<style scoped>
.tft-widget-preview {
  position: relative;
  display: grid;
  place-items: center;
  width: 100%;
  height: 100%;
  overflow: hidden;
  color: rgb(var(--v-theme-on-surface));
}

.tft-widget-preview__text,
.tft-widget-preview__bitmap,
.tft-widget-preview__shape {
  width: 100%;
  height: 100%;
}
</style>
