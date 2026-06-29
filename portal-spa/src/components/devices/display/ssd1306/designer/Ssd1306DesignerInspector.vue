<template>
  <div class="oled-inspector">
    <Ssd1306WidgetPreview
      :widget="widget"
      :display="display"
      :display-scale="2"
      :freeze-render="bitmapPreviewFrozen || bitmapRenderFrozen"
      :preview-text="widgetPreviewText"
      class="oled-inspector__preview"
      :style="previewStyle"
    />
    <v-alert v-if="bitmapError.length > 0" type="error" variant="tonal" density="compact">
      {{ bitmapError }}
    </v-alert>
    <v-select :label="t('device.dialog.ssd1306Display.widgetType')" :items="widgetTypeItems" :model-value="widget.type" @update:model-value="updateWidgetType(String($event))" />
    <v-row>
      <v-col cols="6"><v-text-field :label="t('device.dialog.ssd1306Display.x')" :model-value="widget.x" type="number" min="0" :max="deviceWidth" @update:model-value="updateNumber('x', $event)" /></v-col>
      <v-col cols="6"><v-text-field :label="t('device.dialog.ssd1306Display.y')" :model-value="widget.y" type="number" min="0" :max="deviceHeight" @update:model-value="updateNumber('y', $event)" /></v-col>
      <v-col cols="6">
        <v-text-field v-if="!isBitmapWidget" v-select-on-focus :label="t('device.dialog.ssd1306Display.width')" :model-value="widget.width" type="number" min="1" :max="deviceWidth" @update:model-value="updateNumber('width', $event)" />
        <v-text-field v-else v-select-on-focus :model-value="widget.width" :label="t('device.dialog.ssd1306Display.width')" type="number" min="1" :max="deviceWidth" @focus="beginBitmapResize" @blur="endBitmapResize" @update:model-value="updateBitmapDimension('width', $event)" />
      </v-col>
      <v-col cols="6">
        <v-text-field v-if="!isBitmapWidget" v-select-on-focus :label="t('device.dialog.ssd1306Display.height')" :model-value="widget.height" type="number" min="1" :max="deviceHeight" @update:model-value="updateNumber('height', $event)" />
        <v-text-field v-else v-select-on-focus :model-value="widget.height" :label="t('device.dialog.ssd1306Display.height')" type="number" min="1" :max="deviceHeight" @focus="beginBitmapResize" @blur="endBitmapResize" @update:model-value="updateBitmapDimension('height', $event)" />
      </v-col>
      <v-col v-if="isTextWidget" cols="6"><v-text-field :label="t('device.dialog.ssd1306Display.fontSize')" :model-value="widget.fontSize" type="number" min="1" :max="8" @update:model-value="updateNumber('fontSize', $event)" /></v-col>
      <v-col v-if="supportsStroke" cols="6"><v-text-field :label="t('device.dialog.ssd1306Display.strokeWidth')" :model-value="widget.strokeWidth" type="number" min="1" :max="32" @update:model-value="updateNumber('strokeWidth', $event)" /></v-col>
    </v-row>
    <v-alert v-if="isBitmapWidget" type="info" variant="tonal" density="compact">{{ t('device.dialog.ssd1306Display.bitmapSize', { size: `${widget.width} × ${widget.height}` }) }}</v-alert>
    <v-switch
      v-if="isBitmapWidget"
      :label="t('device.dialog.ssd1306Display.keepAspectRatio')"
      :model-value="bitmapWidget?.keepAspectRatio ?? false"
      density="comfortable"
      inset
      @update:model-value="updateKeepAspectRatio(Boolean($event))"
    />
    <v-alert v-if="isTextWidget" class="oled-inspector__fit" :type="fitInfo.type" variant="tonal" density="compact">
      <div class="oled-inspector__fit-title">{{ fitInfo.title }}</div>
      <div class="text-caption">{{ fitInfo.details }}</div>
    </v-alert>
    <v-text-field v-if="isTextWidget" :label="t('device.dialog.ssd1306Display.text')" :model-value="widget.text" @update:model-value="updateText(String($event))" />
    <MetricPlaceholderBuilder
      v-if="isTextWidget"
      :catalog="metricCatalog"
      :loading="metricsLoading"
      @insert-placeholder="insertMetric"
    />
    <v-alert v-if="isTextWidget && placeholderMessage.length > 0" :type="placeholderTone" variant="tonal" density="compact">
      {{ placeholderMessage }}
    </v-alert>
    <v-text-field
      v-if="isTextWidget && usesMetricPlaceholder"
      :label="t('device.dialog.ssd1306Display.refreshIntervalMs')"
      :hint="t('device.dialog.ssd1306Display.refreshIntervalHint')"
      :model-value="widget.refreshIntervalMs"
      :min="refreshIntervalMinMs"
      :max="refreshIntervalMaxMs"
      type="number"
      persistent-hint
      @update:model-value="updateRefreshInterval"
    />
    <v-file-input v-if="isBitmapWidget" accept="image/*" :label="t('device.dialog.ssd1306Display.bitmapImport')" prepend-icon="upload" density="comfortable" @update:model-value="onBitmapFileSelected" />
    <v-slider v-if="isBitmapWidget" :model-value="bitmapThreshold" :min="0" :max="255" :step="1" hide-details :label="t('device.dialog.ssd1306Display.bitmapThreshold')" @update:model-value="updateBitmapThreshold" />
    <div v-if="isBitmapWidget" class="oled-inspector__bitmap-actions">
      <v-btn variant="text" @click="clearBitmap">{{ t('device.dialog.ssd1306Display.bitmapClear') }}</v-btn>
    </div>
    <v-switch v-if="isTextWidget" :label="t('device.dialog.ssd1306Display.autoSize')" :model-value="widget.autoSize" inset @update:model-value="updateField('autoSize', Boolean($event))" />
    <v-switch v-if="supportsFill" :label="t('device.dialog.ssd1306Display.filled')" :model-value="widget.styleFlags.filled" inset @update:model-value="updateFlag('filled', Boolean($event))" />
    <v-switch :label="t('device.dialog.ssd1306Display.inverted')" :model-value="widget.styleFlags.inverted" inset @update:model-value="updateFlag('inverted', Boolean($event))" />
    <v-switch v-if="isTextWidget" :label="t('device.dialog.ssd1306Display.wrap')" :model-value="widget.styleFlags.wrap" inset @update:model-value="updateFlag('wrap', Boolean($event))" />
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import MetricPlaceholderBuilder from '@/components/devices/display/MetricPlaceholderBuilder.vue'
import Ssd1306WidgetPreview from '@/components/devices/display/ssd1306/Ssd1306WidgetPreview.vue'
import { useDisplayBitmapImportState } from '@/composables/display/useDisplayBitmapImportState'
import { measureSsd1306TextWidget } from '@/components/devices/display/ssd1306/ssd1306-text-layout'
import type { BaseDisplay } from '@/models/devices/display/display'
import { resolveDisplayBitmapDimensionUpdate } from '@/models/devices/display/widgets'
import type { Ssd1306BitmapWidget, Ssd1306Widget } from '@/models/devices/ssd1306/layout'
import { fetchMetricPlaceholders } from '@/api'
import type { MetricPlaceholderDescriptor } from '@/api/contracts'
import {
  metricPlaceholderForDescriptor,
  resolveMetricPlaceholderText,
  validateMetricPlaceholders,
  type MetricPlaceholderValidation,
} from '@/models/metrics/placeholders'
import {
  DISPLAY_WIDGET_REFRESH_INTERVAL_DEFAULT_MS,
  DISPLAY_WIDGET_REFRESH_INTERVAL_DISABLED,
  DISPLAY_WIDGET_REFRESH_INTERVAL_MAX_MS,
  DISPLAY_WIDGET_REFRESH_INTERVAL_MIN_MS,
} from '@/models/devices/display/layout-normalizer'

const props = defineProps<{ widget: Ssd1306Widget; deviceWidth: number; deviceHeight: number; bitmapRenderFrozen?: boolean; display: BaseDisplay<'mono1'> }>()
const emit = defineEmits<{
  'update-widget': [patch: Partial<Ssd1306Widget>]
  'bitmap-resize-start': [widgetId: string]
  'bitmap-resize-end': [widgetId: string]
}>()
const { t } = useI18n()

const widgetTypeItems = ['text', 'bitmap', 'rect', 'line', 'circle', 'ellipse'].map(value => ({ title: t(`device.dialog.ssd1306Display.widgetTypes.${value}`), value }))
const isTextWidget = computed(() => props.widget.type === 'text')
const isBitmapWidget = computed(() => props.widget.type === 'bitmap')
const supportsFill = computed(() => props.widget.type === 'rect' || props.widget.type === 'circle' || props.widget.type === 'ellipse')
const supportsStroke = computed(() => props.widget.type === 'rect' || props.widget.type === 'line' || props.widget.type === 'circle' || props.widget.type === 'ellipse')
const previewStyle = computed(() => ({
  width: `${Math.max(48, Math.round(props.widget.width * 2))}px`,
  height: `${Math.max(isBitmapWidget.value ? 48 : 24, Math.round(props.widget.height * 2))}px`,
}))
const fitInfo = computed<{ type: 'info' | 'success' | 'warning'; title: string; details: string }>(() => {
  if (props.widget.type !== 'text') return { type: 'info' as const, title: '', details: '' }
  const measurement = measureSsd1306TextWidget(props.widget)
  return {
    type: measurement.fits ? 'success' : 'warning',
    title: measurement.fits ? t('device.dialog.ssd1306Display.fits') : props.widget.styleFlags.wrap ? t('device.dialog.ssd1306Display.clipsWrapped') : t('device.dialog.ssd1306Display.clips'),
    details: props.widget.styleFlags.wrap && measurement.wrappedLines > 1
      ? t('device.dialog.ssd1306Display.sizeWrap', { needed: `${measurement.measuredWidth} × ${measurement.measuredHeight}`, box: `${measurement.boxWidth} × ${measurement.boxHeight}`, lines: measurement.wrappedLines })
      : t('device.dialog.ssd1306Display.sizeFit', { needed: `${measurement.measuredWidth} × ${measurement.measuredHeight}`, box: `${measurement.boxWidth} × ${measurement.boxHeight}` }),
  }
})

const bitmapWorkflow = useDisplayBitmapImportState(
  () => props.widget,
  patch => emit('update-widget', patch),
  key => t(key),
  (file, width, height, threshold) => props.display.importBitmapFromFile(file, width, height, threshold),
  (width, height) => props.display.createBitmapPlaceholder(width, height).bitmapData,
  () => props.widget.type === 'bitmap',
)

const bitmapError = bitmapWorkflow.bitmapError
const bitmapThreshold = bitmapWorkflow.bitmapThreshold
const bitmapPreviewFrozen = bitmapWorkflow.bitmapPreviewFrozen
const bitmapWidget = computed<Ssd1306BitmapWidget | null>(() => (isBitmapWidget.value ? props.widget as Ssd1306BitmapWidget : null))
const metricCatalog = ref<MetricPlaceholderDescriptor[]>([])
const metricsLoading = ref(false)
const placeholderValidation = ref<MetricPlaceholderValidation>({ status: 'static', parsed: null, descriptor: null })
const refreshIntervalMinMs = DISPLAY_WIDGET_REFRESH_INTERVAL_MIN_MS
const refreshIntervalMaxMs = DISPLAY_WIDGET_REFRESH_INTERVAL_MAX_MS
let validationTimer: ReturnType<typeof setTimeout> | null = null
let refreshTimer: ReturnType<typeof setInterval> | null = null

const placeholderTone = computed<'success' | 'warning' | 'error' | 'info'>(() => {
  switch (placeholderValidation.value.status) {
    case 'valid':
      return 'success'
    case 'unavailable':
      return 'warning'
    case 'invalid':
    case 'multiple':
      return 'error'
    default:
      return 'info'
  }
})
const placeholderMessage = computed(() => {
  switch (placeholderValidation.value.status) {
    case 'valid':
      return t('device.dialog.ssd1306Display.placeholderValid')
    case 'unavailable':
      return t('device.dialog.ssd1306Display.placeholderUnavailable')
    case 'invalid':
      return t('device.dialog.ssd1306Display.placeholderInvalid')
    case 'multiple':
      return t('device.dialog.ssd1306Display.placeholderMultiple')
    default:
      return ''
  }
})
const usesMetricPlaceholder = computed(() => placeholderValidation.value.status === 'valid' || placeholderValidation.value.status === 'unavailable' || props.widget.bindingKind === 'metric')
const widgetPreviewText = computed(() => props.widget.type === 'text' ? resolveMetricPlaceholderText(props.widget.text, metricCatalog.value) : undefined)

onMounted(() => {
  void loadMetricCatalog()
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
watch(() => props.widget.refreshIntervalMs, () => restartMetricRefresh())
watch(isTextWidget, () => restartMetricRefresh())

function updateField<K extends keyof Ssd1306Widget>(key: K, value: Ssd1306Widget[K]): void { emit('update-widget', { [key]: value } as Partial<Ssd1306Widget>) }
function updateKeepAspectRatio(value: boolean): void {
  if (props.widget.type !== 'bitmap') {
    return
  }
  emit('update-widget', { keepAspectRatio: value } as Partial<Ssd1306Widget>)
}
function updateWidgetType(value: string): void {
  if (!['text', 'bitmap', 'rect', 'line', 'circle', 'ellipse'].includes(value)) {
    return
  }
  if (value === 'bitmap') {
    emit('update-widget', {
      type: 'bitmap',
      bitmapData: props.display.createBitmapPlaceholder(props.widget.width, props.widget.height).bitmapData,
      bitmapFormat: props.display.bitmapFormat,
      keepAspectRatio: false,
    } as Partial<Ssd1306Widget>)
    return
  }
  emit('update-widget', { type: value as Ssd1306Widget['type'] })
}
function updateFlag(key: keyof Ssd1306Widget['styleFlags'], value: boolean): void { emit('update-widget', { styleFlags: { ...props.widget.styleFlags, [key]: value } }) }

function updateNumber(key: keyof Pick<Ssd1306Widget, 'x' | 'y' | 'width' | 'height' | 'fontSize' | 'strokeWidth'>, value: string | number): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) return
  emit('update-widget', { [key]: Math.round(numeric) } as Partial<Ssd1306Widget>)
}

function normalizeRefreshInterval(value: string | number): number {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return DISPLAY_WIDGET_REFRESH_INTERVAL_DEFAULT_MS
  }
  return Math.min(DISPLAY_WIDGET_REFRESH_INTERVAL_MAX_MS, Math.max(DISPLAY_WIDGET_REFRESH_INTERVAL_MIN_MS, Math.round(numeric)))
}

function updateRefreshInterval(value: string | number): void {
  emit('update-widget', { refreshIntervalMs: normalizeRefreshInterval(value) } as Partial<Ssd1306Widget>)
}

async function loadMetricCatalog(): Promise<void> {
  metricsLoading.value = true
  try {
    metricCatalog.value = (await fetchMetricPlaceholders()).placeholders
    runPlaceholderValidation(props.widget.text)
  } finally {
    metricsLoading.value = false
  }
}

function schedulePlaceholderValidation(text: string): void {
  if (validationTimer !== null) {
    clearTimeout(validationTimer)
  }
  validationTimer = setTimeout(() => runPlaceholderValidation(text), 350)
}

function runPlaceholderValidation(text: string): void {
  placeholderValidation.value = validateMetricPlaceholders(text, metricCatalog.value)
}

function updateText(text: string): void {
  const validation = validateMetricPlaceholders(text, metricCatalog.value)
  const patch: Partial<Ssd1306Widget> = { text }
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

function insertMetric(metric: MetricPlaceholderDescriptor): void {
  const text = metricPlaceholderForDescriptor(metric)
  emit('update-widget', {
    text,
    bindingKind: 'metric',
    metricNamespace: metric.namespace,
    sourceDeviceId: metric.sourceId,
    metricId: metric.metricId,
    refreshIntervalMs: props.widget.refreshIntervalMs > 0 ? props.widget.refreshIntervalMs : DISPLAY_WIDGET_REFRESH_INTERVAL_DEFAULT_MS,
  } as Partial<Ssd1306Widget>)
  placeholderValidation.value = validateMetricPlaceholders(text, metricCatalog.value)
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
  if (!isTextWidget.value || props.widget.refreshIntervalMs <= 0 || validateMetricPlaceholders(props.widget.text, metricCatalog.value).status === 'static') {
    return
  }
  refreshTimer = setInterval(() => {
    void loadMetricCatalog()
  }, props.widget.refreshIntervalMs)
}

function updateBitmapDimension(key: 'width' | 'height', value: string | number): void {
  const currentWidget = bitmapWidget.value
  if (currentWidget === null) {
    return
  }
  const nextSize = resolveDisplayBitmapDimensionUpdate(currentWidget, key, value)
  if (nextSize !== null) {
    emit('update-widget', nextSize as Partial<Ssd1306Widget>)
  }
}

function beginBitmapResize(): void {
  const currentWidget = bitmapWidget.value
  if (currentWidget !== null) {
    emit('bitmap-resize-start', currentWidget.id)
  }
}

function endBitmapResize(): void {
  const currentWidget = bitmapWidget.value
  if (currentWidget !== null) {
    emit('bitmap-resize-end', currentWidget.id)
  }
}

function onBitmapFileSelected(value: File | File[] | null): void {
  if (value === null || (Array.isArray(value) && value.length === 0)) {
    clearBitmap()
    return
  }
  void bitmapWorkflow.queueBitmapImport(value)
}
function updateBitmapThreshold(value: string | number): void { bitmapWorkflow.setBitmapThreshold(value) }
function clearBitmap(): void { bitmapWorkflow.clearBitmap() }

</script>

<style scoped>
.oled-inspector { display: grid; gap: 12px; }
.oled-inspector__fit { margin-top: -4px; }
.oled-inspector__fit-title { font-weight: 600; }
.oled-inspector__preview { justify-self: start; border: 1px solid rgb(var(--v-theme-outline-variant)); border-radius: 0; }
.oled-inspector__preview { margin-bottom: 8px; }
</style>
