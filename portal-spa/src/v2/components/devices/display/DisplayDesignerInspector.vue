<template>
  <div>
    <v-alert v-if="bitmapError.length > 0" type="error" variant="tonal" density="comfortable" class="mb-3">
      {{ bitmapError }}
    </v-alert>

    <!-- Geometry -->
    <v-row density="comfortable">
      <v-col cols="12">
        <v-select
          density="compact"
          variant="outlined"
          hide-details
          :label="t('device.dialog.display.widgetType')"
          :items="widgetTypeItems"
          :model-value="widget.type"
          @update:model-value="updateWidgetType(String($event))"
        />
      </v-col>
      <v-col cols="6">
        <v-text-field
          density="compact"
          variant="outlined"
          hide-details
          :label="t('device.dialog.display.x')"
          :model-value="widget.x"
          type="number"
          min="0"
          :max="deviceWidth"
          @update:model-value="updateNumber('x', $event)"
        />
      </v-col>
      <v-col cols="6">
        <v-text-field
          density="compact"
          variant="outlined"
          hide-details
          :label="t('device.dialog.display.y')"
          :model-value="widget.y"
          type="number"
          min="0"
          :max="deviceHeight"
          @update:model-value="updateNumber('y', $event)"
        />
      </v-col>
      <v-col cols="6">
        <v-text-field
          v-if="!isBitmapWidget"
          density="compact"
          variant="outlined"
          hide-details
          :label="t('device.dialog.display.width')"
          :model-value="widget.width"
          type="number"
          min="1"
          :max="deviceWidth"
          @update:model-value="updateNumber('width', $event)"
        />
        <v-text-field
          v-else
          density="compact"
          variant="outlined"
          hide-details
          :label="t('device.dialog.display.width')"
          :model-value="widget.width"
          type="number"
          min="1"
          :max="deviceWidth"
          @focus="beginBitmapResize"
          @blur="endBitmapResize"
          @update:model-value="updateBitmapDimension('width', $event)"
        />
      </v-col>
      <v-col cols="6">
        <v-text-field
          v-if="!isBitmapWidget"
          density="compact"
          variant="outlined"
          hide-details
          :label="t('device.dialog.display.height')"
          :model-value="widget.height"
          type="number"
          min="1"
          :max="deviceHeight"
          @update:model-value="updateNumber('height', $event)"
        />
        <v-text-field
          v-else
          density="compact"
          variant="outlined"
          hide-details
          :label="t('device.dialog.display.height')"
          :model-value="widget.height"
          type="number"
          min="1"
          :max="deviceHeight"
          @focus="beginBitmapResize"
          @blur="endBitmapResize"
          @update:model-value="updateBitmapDimension('height', $event)"
        />
      </v-col>
      <v-col v-if="isTextWidget" cols="6">
        <v-text-field
          density="compact"
          variant="outlined"
          hide-details
          :label="t('device.dialog.display.fontSize')"
          :model-value="widget.fontSize"
          type="number"
          min="1"
          :max="8"
          @update:model-value="updateNumber('fontSize', $event)"
        />
      </v-col>
      <v-col v-if="supportsStroke" cols="6">
        <v-text-field
          density="compact"
          variant="outlined"
          hide-details
          :label="t('device.dialog.display.strokeWidth')"
          :model-value="widget.strokeWidth"
          type="number"
          min="1"
          :max="32"
          @update:model-value="updateNumber('strokeWidth', $event)"
        />
      </v-col>
    </v-row>

    <!-- Text -->
    <v-row v-if="isTextWidget" density="comfortable" class="mt-1">
      <v-col cols="12">
        <v-textarea
          :label="t('device.dialog.display.text')"
          :model-value="widget.text"
          density="compact"
          variant="outlined"
          auto-grow
          rows="2"
          hide-details
          @update:model-value="updateText(String($event))"
        />
      </v-col>
      <v-col cols="12">
        <v-chip size="small" variant="tonal" :color="fitInfo.type" class="me-2">
          {{ fitInfo.title }}
        </v-chip>
        <span class="text-body-small text-medium-emphasis">{{ fitInfo.details }}</span>
      </v-col>
      <v-col v-if="placeholderMessage.length > 0" cols="12">
        <v-chip size="small" variant="tonal" :color="placeholderTone" class="me-2 mb-2">
          {{ placeholderMessage }}
        </v-chip>
        <v-text-field
          v-if="usesMetricPlaceholder"
          :label="t('device.dialog.display.refreshIntervalMs')"
          :hint="t('device.dialog.display.refreshIntervalHint')"
          :model-value="widget.refreshIntervalMs"
          :min="refreshIntervalMinMs"
          :max="refreshIntervalMaxMs"
          density="compact"
          variant="outlined"
          type="number"
          persistent-hint
          @update:model-value="updateRefreshInterval"
        />
      </v-col>
    </v-row>

    <MetricPlaceholderBuilder
      v-if="isTextWidget"
      class="mt-3"
      :catalog="metricCatalog"
      :loading="metricsLoading"
      :surface="true"
    />

    <!-- Bitmap -->
    <v-row v-if="isBitmapWidget" density="comfortable" class="mt-1">
      <v-col cols="12">
        <v-chip size="small" variant="tonal" color="info">
          {{ t('device.dialog.display.bitmapSize', { size: `${widget.width} × ${widget.height}` }) }}
        </v-chip>
      </v-col>
      <v-col cols="12">
        <v-switch
          :label="t('device.dialog.display.keepAspectRatio')"
          :model-value="bitmapWidget?.keepAspectRatio ?? false"
          density="compact"
          hide-details
          inset
          @update:model-value="updateKeepAspectRatio(Boolean($event))"
        />
      </v-col>
      <v-col cols="12">
        <v-file-input
          accept="image/*"
          :label="t('device.dialog.display.bitmapImport')"
          prepend-icon="upload"
          density="compact"
          variant="outlined"
          hide-details
          @update:model-value="onBitmapFileSelected"
        />
      </v-col>
      <v-col cols="12">
        <v-slider
          :model-value="bitmapThreshold"
          :min="0"
          :max="255"
          :step="1"
          hide-details
          :label="t('device.dialog.display.bitmapThreshold')"
          @update:model-value="updateBitmapThreshold"
        />
      </v-col>
      <v-col cols="12">
        <v-btn variant="text" size="small" @click="clearBitmap">
          {{ t('device.dialog.display.bitmapClear') }}
        </v-btn>
      </v-col>
    </v-row>

    <!-- Toggles -->
    <v-row density="comfortable" class="mt-1">
      <v-col v-if="isTextWidget" cols="12">
        <v-switch
          :label="t('device.dialog.display.autoSize')"
          :model-value="widget.autoSize"
          density="compact"
          hide-details
          inset
          @update:model-value="updateField('autoSize', Boolean($event))"
        />
      </v-col>
      <v-col v-if="supportsFill" cols="12">
        <v-switch
          :label="t('device.dialog.display.filled')"
          :model-value="widget.styleFlags.filled"
          density="compact"
          hide-details
          inset
          @update:model-value="updateFlag('filled', Boolean($event))"
        />
      </v-col>
      <v-col cols="12">
        <v-switch
          :label="t('device.dialog.display.inverted')"
          :model-value="widget.styleFlags.inverted"
          density="compact"
          hide-details
          inset
          @update:model-value="updateFlag('inverted', Boolean($event))"
        />
      </v-col>
      <v-col v-if="isTextWidget" cols="12">
        <v-switch
          :label="t('device.dialog.display.wrap')"
          :model-value="widget.styleFlags.wrap"
          density="compact"
          hide-details
          inset
          @update:model-value="updateFlag('wrap', Boolean($event))"
        />
      </v-col>
    </v-row>
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type { MetricPlaceholderDescriptor } from '@/api/contracts'
import type { BaseDisplay } from '@/models/devices/display/display'
import type { DisplayBitmapWidget, DisplayWidget, DisplayWidgetType } from '@/models/devices/display/layout'
import {
  DISPLAY_WIDGET_REFRESH_INTERVAL_DEFAULT_MS,
  DISPLAY_WIDGET_REFRESH_INTERVAL_DISABLED,
  DISPLAY_WIDGET_REFRESH_INTERVAL_MAX_MS,
  DISPLAY_WIDGET_REFRESH_INTERVAL_MIN_MS,
} from '@/models/devices/display/layout-normalizer'
import { measureSsd1306TextWidget } from '@/models/devices/display/text/ssd1306-text-layout'
import { resolveDisplayBitmapDimensionUpdate } from '@/models/devices/display/widgets'
import {
  resolveMetricPlaceholderText,
  validateMetricPlaceholders,
  type MetricPlaceholderValidation,
} from '@/models/metrics/placeholders'
import type { RasterImageFormat } from '@/raster/raster-image-types'

import MetricPlaceholderBuilder from './MetricPlaceholderBuilder.vue'

const props = defineProps<{
  widget: DisplayWidget
  deviceWidth: number
  deviceHeight: number
  display: BaseDisplay<RasterImageFormat>
  metricCatalog: readonly MetricPlaceholderDescriptor[]
  metricsLoading?: boolean
  refreshMetricCatalog: () => Promise<void>
}>()

const emit = defineEmits<{
  'update-widget': [patch: Partial<DisplayWidget>]
  'bitmap-resize-start': [widgetId: string]
  'bitmap-resize-end': [widgetId: string]
}>()

const { t } = useI18n()

const widgetTypeItems: Array<{ title: string; value: DisplayWidgetType }> = (['text', 'bitmap', 'rect', 'line', 'circle', 'ellipse'] as DisplayWidgetType[]).map(value => ({
  title: t(`device.dialog.display.widgetTypes.${value}`),
  value,
}))

const isTextWidget = computed(() => props.widget.type === 'text')
const isBitmapWidget = computed(() => props.widget.type === 'bitmap')
const supportsFill = computed(() => props.widget.type === 'rect' || props.widget.type === 'circle' || props.widget.type === 'ellipse')
const supportsStroke = computed(() => props.widget.type === 'rect' || props.widget.type === 'line' || props.widget.type === 'circle' || props.widget.type === 'ellipse')
const resolvedPreviewText = computed(() => (isTextWidget.value ? resolveMetricPlaceholderText(props.widget.text, props.metricCatalog) : ''))

const fitInfo = computed<{ type: 'info' | 'success' | 'warning'; title: string; details: string }>(() => {
  if (props.widget.type !== 'text') return { type: 'info', title: '', details: '' }
  const measurement = measureSsd1306TextWidget(props.widget, { text: resolvedPreviewText.value })
  return {
    type: measurement.fits ? 'success' : 'warning',
    title: measurement.fits
      ? t('device.dialog.display.fits')
      : props.widget.styleFlags.wrap
        ? t('device.dialog.display.clipsWrapped')
        : t('device.dialog.display.clips'),
    details: props.widget.styleFlags.wrap && measurement.wrappedLines > 1
      ? t('device.dialog.display.sizeWrap', { needed: `${measurement.measuredWidth} × ${measurement.measuredHeight}`, box: `${measurement.boxWidth} × ${measurement.boxHeight}`, lines: measurement.wrappedLines })
      : t('device.dialog.display.sizeFit', { needed: `${measurement.measuredWidth} × ${measurement.measuredHeight}`, box: `${measurement.boxWidth} × ${measurement.boxHeight}` }),
  }
})

const bitmapError = ref('')
const bitmapThreshold = ref(128)
const bitmapWidget = computed<DisplayBitmapWidget | null>(() => (isBitmapWidget.value ? props.widget as DisplayBitmapWidget : null))

const placeholderValidation = ref<MetricPlaceholderValidation>(validateMetricPlaceholders('', []))
const refreshIntervalMinMs = DISPLAY_WIDGET_REFRESH_INTERVAL_MIN_MS
const refreshIntervalMaxMs = DISPLAY_WIDGET_REFRESH_INTERVAL_MAX_MS
let validationTimer: ReturnType<typeof setTimeout> | null = null
let refreshTimer: ReturnType<typeof setInterval> | null = null

const placeholderTone = computed<'warning' | 'error' | 'info'>(() => {
  switch (placeholderValidation.value.status) {
    case 'unavailable':
      return 'warning'
    case 'invalid':
      return 'error'
    default:
      return 'info'
  }
})

const placeholderMessage = computed(() => {
  switch (placeholderValidation.value.status) {
    case 'unavailable':
      return t('device.dialog.display.placeholderUnavailable')
    case 'invalid':
      return t('device.dialog.display.placeholderInvalid')
    default:
      return ''
  }
})

const usesMetricPlaceholder = computed(() => placeholderValidation.value.status === 'valid' || placeholderValidation.value.status === 'unavailable' || props.widget.bindingKind === 'metric')

onMounted(() => {
  runPlaceholderValidation(props.widget.text)
  restartMetricRefresh()
})

onBeforeUnmount(() => {
  if (validationTimer !== null) {
    clearTimeout(validationTimer)
    validationTimer = null
  }
  stopMetricRefresh()
})

watch(() => props.widget.text, text => {
  schedulePlaceholderValidation(text)
  restartMetricRefresh()
})
watch(() => props.metricCatalog, () => runPlaceholderValidation(props.widget.text))
watch(() => props.widget.refreshIntervalMs, () => restartMetricRefresh())
watch(isTextWidget, () => restartMetricRefresh())

function updateField<K extends keyof DisplayWidget>(key: K, value: DisplayWidget[K]): void {
  emit('update-widget', { [key]: value } as Partial<DisplayWidget>)
}

function updateKeepAspectRatio(value: boolean): void {
  if (props.widget.type !== 'bitmap') return
  emit('update-widget', { keepAspectRatio: value } as Partial<DisplayWidget>)
}

function updateWidgetType(value: string): void {
  if (!['text', 'bitmap', 'rect', 'line', 'circle', 'ellipse'].includes(value)) return
  if (value === 'bitmap') {
    emit('update-widget', {
      type: 'bitmap',
      bitmapData: props.display.createBitmapPlaceholder(props.widget.width, props.widget.height).bitmapData,
      bitmapFormat: props.display.bitmapFormat,
      keepAspectRatio: false,
    } as Partial<DisplayWidget>)
    return
  }
  emit('update-widget', { type: value as DisplayWidget['type'] })
}

function updateFlag(key: keyof DisplayWidget['styleFlags'], value: boolean): void {
  emit('update-widget', { styleFlags: { ...props.widget.styleFlags, [key]: value } })
}

function updateNumber(key: keyof Pick<DisplayWidget, 'x' | 'y' | 'width' | 'height' | 'fontSize' | 'strokeWidth'>, value: string | number): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) return
  emit('update-widget', { [key]: Math.round(numeric) } as Partial<DisplayWidget>)
}

function normalizeRefreshInterval(value: string | number): number {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) return DISPLAY_WIDGET_REFRESH_INTERVAL_DEFAULT_MS
  return Math.min(DISPLAY_WIDGET_REFRESH_INTERVAL_MAX_MS, Math.max(DISPLAY_WIDGET_REFRESH_INTERVAL_MIN_MS, Math.round(numeric)))
}

function updateRefreshInterval(value: string | number): void {
  emit('update-widget', { refreshIntervalMs: normalizeRefreshInterval(value) } as Partial<DisplayWidget>)
}

async function loadMetricCatalog(): Promise<void> {
  await props.refreshMetricCatalog()
}

function schedulePlaceholderValidation(text: string): void {
  if (validationTimer !== null) clearTimeout(validationTimer)
  validationTimer = setTimeout(() => runPlaceholderValidation(text), 350)
}

function runPlaceholderValidation(text: string): void {
  placeholderValidation.value = validateMetricPlaceholders(text, props.metricCatalog)
}

function updateText(text: string): void {
  const validation = validateMetricPlaceholders(text, props.metricCatalog)
  const patch: Partial<DisplayWidget> = { text }
  if (validation.status === 'static') {
    patch.bindingKind = 'constant_text'
    patch.metricNamespace = 'dev'
    patch.sourceDeviceId = 0
    patch.metricId = 0
    patch.refreshIntervalMs = DISPLAY_WIDGET_REFRESH_INTERVAL_DISABLED
  } else if (validation.descriptor !== null) {
    patch.bindingKind = 'metric'
    patch.metricNamespace = validation.descriptor.namespace
    patch.sourceDeviceId = validation.descriptor.sourceId
    patch.metricId = validation.descriptor.metricId
    patch.refreshIntervalMs = props.widget.refreshIntervalMs > 0 ? props.widget.refreshIntervalMs : DISPLAY_WIDGET_REFRESH_INTERVAL_DEFAULT_MS
  } else if (validation.parsed !== null) {
    patch.bindingKind = 'metric'
    patch.metricNamespace = validation.parsed.namespace
    patch.sourceDeviceId = validation.parsed.sourceId
    patch.refreshIntervalMs = props.widget.refreshIntervalMs > 0 ? props.widget.refreshIntervalMs : DISPLAY_WIDGET_REFRESH_INTERVAL_DEFAULT_MS
  }
  emit('update-widget', patch)
  placeholderValidation.value = validation
  restartMetricRefresh()
}

function stopMetricRefresh(): void {
  if (refreshTimer !== null) {
    clearInterval(refreshTimer)
    refreshTimer = null
  }
}

function restartMetricRefresh(): void {
  stopMetricRefresh()
  if (!isTextWidget.value || props.widget.refreshIntervalMs <= 0 || validateMetricPlaceholders(props.widget.text, props.metricCatalog).status === 'static') return
  refreshTimer = setInterval(() => {
    void loadMetricCatalog()
  }, props.widget.refreshIntervalMs)
}

function updateBitmapDimension(key: 'width' | 'height', value: string | number): void {
  const currentWidget = bitmapWidget.value
  if (currentWidget === null) return
  const nextSize = resolveDisplayBitmapDimensionUpdate(currentWidget, key, value)
  if (nextSize !== null) emit('update-widget', nextSize as Partial<DisplayWidget>)
}

function beginBitmapResize(): void {
  const currentWidget = bitmapWidget.value
  if (currentWidget !== null) emit('bitmap-resize-start', currentWidget.id)
}

function endBitmapResize(): void {
  const currentWidget = bitmapWidget.value
  if (currentWidget !== null) emit('bitmap-resize-end', currentWidget.id)
}

async function onBitmapFileSelectedImpl(file: File): Promise<void> {
  try {
    const result = await props.display.importBitmapFromFile(file, props.widget.width, props.widget.height, bitmapThreshold.value)
    emit('update-widget', { bitmapData: result.imageData })
  } catch (error) {
    bitmapError.value = error instanceof Error ? error.message : 'Failed to import bitmap'
  }
}

function onBitmapFileSelected(value: File | File[] | null): void {
  if (value === null || (Array.isArray(value) && value.length === 0)) {
    clearBitmap()
    return
  }
  const file = Array.isArray(value) ? value[0] : value
  if (file) void onBitmapFileSelectedImpl(file)
}

function updateBitmapThreshold(value: string | number): void {
  bitmapThreshold.value = Number(value)
}

function clearBitmap(): void {
  const placeholder = props.display.createBitmapPlaceholder(props.widget.width, props.widget.height)
  emit('update-widget', { bitmapData: placeholder.bitmapData })
}
</script>
