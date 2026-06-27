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
    <canvas
      v-else-if="widget.type === 'bitmap'"
      ref="bitmapCanvasRef"
      class="oled-widget-preview__bitmap"
      aria-hidden="true"
    />
    <canvas
      v-else-if="isShapeWidget"
      ref="shapeCanvasRef"
      class="oled-widget-preview__shape"
      aria-hidden="true"
    >
    </canvas>
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
const bitmapCanvasRef = ref<HTMLCanvasElement | null>(null)
const shapeCanvasRef = ref<HTMLCanvasElement | null>(null)
let resizeObserver: ResizeObserver | null = null
let drawFrame = 0

const iconName = computed(() => (props.widget.text.trim().length > 0 ? props.widget.text : 'oled-icon'))
const displayScale = computed(() => Math.max(1, props.displayScale ?? 1))
const iconSize = computed(() => resolveOledDisplayIconSize(displayScale.value))
const isShapeWidget = computed(() => props.widget.type === 'rect' || props.widget.type === 'line' || props.widget.type === 'circle' || props.widget.type === 'ellipse')
const isBitmapWidget = computed(() => props.widget.type === 'bitmap')
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

function drawBitmapCanvas(): void {
  const canvas = bitmapCanvasRef.value
  if (canvas === null || !isBitmapWidget.value) {
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

  const image = context.createImageData(canvasSize.bitmapWidth, canvasSize.bitmapHeight)
  let bytes = new Uint8Array(canvasSize.bitmapHeight * Math.ceil(canvasSize.bitmapWidth / 8))
  try {
    bytes = Uint8Array.from(globalThis.atob(typeof props.widget.bitmapData === 'string' ? props.widget.bitmapData : ''), char => char.charCodeAt(0))
  } catch {
    // keep default blank data
  }
  const inverted = props.widget.styleFlags.inverted
  const rowBytes = Math.ceil(canvasSize.bitmapWidth / 8)
  for (let y = 0; y < canvasSize.bitmapHeight; y += 1) {
    const rowOffset = y * rowBytes
    for (let x = 0; x < canvasSize.bitmapWidth; x += 1) {
      const byte = bytes[rowOffset + Math.floor(x / 8)] ?? 0
      const bit = (byte >> (7 - (x % 8))) & 1
      const on = inverted ? bit === 0 : bit === 1
      const index = (y * canvasSize.bitmapWidth + x) * 4
      const value = on ? 255 : 0
      image.data[index] = value
      image.data[index + 1] = value
      image.data[index + 2] = value
      image.data[index + 3] = 255
    }
  }
  context.putImageData(image, 0, 0)
}

function drawShapeCanvas(): void {
  const canvas = shapeCanvasRef.value
  if (canvas === null || !isShapeWidget.value) {
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

  const inverted = props.widget.styleFlags.inverted
  const color = inverted ? oledOffColor : oledOnColor
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
  () => [props.widget.type, props.widget.width, props.widget.height, props.widget.type === 'bitmap' && typeof props.widget.bitmapData === 'string' ? props.widget.bitmapData : '', props.widget.styleFlags.inverted],
  async () => {
    await nextTick()
    drawBitmapCanvas()
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
  background: transparent;
  color: rgb(var(--v-theme-on-surface));
}

.oled-widget-preview--text {
  background: rgb(0 0 0);
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
  display: block;
  width: 100%;
  height: 100%;
  max-width: 100%;
  max-height: 100%;
  image-rendering: pixelated;
}

.oled-widget-preview__bitmap {
  display: block;
  width: 100%;
  height: 100%;
  max-width: 100%;
  max-height: 100%;
  image-rendering: pixelated;
}

</style>
