<template>
  <div class="oled-inspector">
    <Ssd1306WidgetPreview
      :widget="widget"
      :display-scale="2"
      :freeze-render="bitmapPreviewFrozen || bitmapRenderFrozen"
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
        <v-text-field v-if="!isBitmapWidget" :label="t('device.dialog.ssd1306Display.width')" :model-value="widget.width" type="number" min="1" :max="deviceWidth" @update:model-value="updateNumber('width', $event)" />
        <v-text-field v-else :model-value="bitmapWidthField" :label="t('device.dialog.ssd1306Display.width')" type="number" min="1" :max="deviceWidth" @update:model-value="updateBitmapDimension('width', $event)" />
      </v-col>
      <v-col cols="6">
        <v-text-field v-if="!isBitmapWidget" :label="t('device.dialog.ssd1306Display.height')" :model-value="widget.height" type="number" min="1" :max="deviceHeight" @update:model-value="updateNumber('height', $event)" />
        <v-text-field v-else :model-value="bitmapHeightField" :label="t('device.dialog.ssd1306Display.height')" type="number" min="1" :max="deviceHeight" @update:model-value="updateBitmapDimension('height', $event)" />
      </v-col>
      <v-col v-if="isTextWidget" cols="6"><v-text-field :label="t('device.dialog.ssd1306Display.fontSize')" :model-value="widget.fontSize" type="number" min="1" :max="8" @update:model-value="updateNumber('fontSize', $event)" /></v-col>
      <v-col cols="6"><v-text-field :label="t('device.dialog.ssd1306Display.strokeWidth')" :model-value="widget.strokeWidth" type="number" min="1" :max="32" @update:model-value="updateNumber('strokeWidth', $event)" /></v-col>
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
    <v-text-field v-if="isTextWidget" :label="t('device.dialog.ssd1306Display.text')" :model-value="widget.text" @update:model-value="updateField('text', String($event))" />
    <v-file-input v-if="isBitmapWidget" accept="image/*" :label="t('device.dialog.ssd1306Display.bitmapImport')" prepend-icon="upload" density="comfortable" @update:model-value="onBitmapFileSelected" />
    <v-slider v-if="isBitmapWidget" :model-value="bitmapThreshold" :min="0" :max="255" :step="1" hide-details :label="t('device.dialog.ssd1306Display.bitmapThreshold')" @update:model-value="updateBitmapThreshold" />
    <div v-if="isBitmapWidget" class="oled-inspector__bitmap-actions">
      <v-btn variant="text" @click="clearBitmap">{{ t('device.dialog.ssd1306Display.bitmapClear') }}</v-btn>
    </div>
    <v-select v-if="isTextWidget" :label="t('device.dialog.ssd1306Display.bindingKind')" :items="bindingKindItems" :model-value="widget.bindingKind" @update:model-value="updateBindingKind(String($event))" />
    <v-row v-if="isTextWidget">
      <v-col cols="6"><v-text-field :label="t('device.dialog.ssd1306Display.sourceDeviceId')" :model-value="widget.sourceDeviceId" type="number" min="0" @update:model-value="updateNumber('sourceDeviceId', $event)" /></v-col>
      <v-col cols="6"><v-text-field :label="t('device.dialog.ssd1306Display.metricId')" :model-value="widget.metricId" type="number" @update:model-value="updateNumber('metricId', $event)" /></v-col>
    </v-row>
    <v-switch v-if="isTextWidget" :label="t('device.dialog.ssd1306Display.autoSize')" :model-value="widget.autoSize" inset @update:model-value="updateField('autoSize', Boolean($event))" />
    <v-switch v-if="supportsFill" :label="t('device.dialog.ssd1306Display.filled')" :model-value="widget.styleFlags.filled" inset @update:model-value="updateFlag('filled', Boolean($event))" />
    <v-switch :label="t('device.dialog.ssd1306Display.inverted')" :model-value="widget.styleFlags.inverted" inset @update:model-value="updateFlag('inverted', Boolean($event))" />
    <v-switch v-if="isTextWidget" :label="t('device.dialog.ssd1306Display.wrap')" :model-value="widget.styleFlags.wrap" inset @update:model-value="updateFlag('wrap', Boolean($event))" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import Ssd1306WidgetPreview from '@/components/devices/display/ssd1306/Ssd1306WidgetPreview.vue'
import { useDisplayBitmapImportState } from '@/composables/display/useDisplayBitmapImportState'
import { measureSsd1306TextWidget } from '@/components/devices/display/ssd1306/ssd1306-text-layout'
import { createSsd1306BitmapPlaceholder, importSsd1306BitmapFromFile } from '@/components/devices/display/ssd1306/ssd1306-bitmap'
import type { Ssd1306BitmapWidget, Ssd1306Widget } from '@/models/devices/ssd1306/layout'

const props = defineProps<{ widget: Ssd1306Widget; deviceWidth: number; deviceHeight: number; bitmapRenderFrozen?: boolean }>()
const emit = defineEmits<{ 'update-widget': [patch: Partial<Ssd1306Widget>] }>()
const { t } = useI18n()

const widgetTypeItems = ['text', 'bitmap', 'rect', 'line', 'circle', 'ellipse'].map(value => ({ title: t(`device.dialog.ssd1306Display.widgetTypes.${value}`), value }))
const bindingKindItems = ['unbound', 'device', 'metric', 'constant_text'].map(value => ({ title: t(`device.dialog.ssd1306Display.bindingKinds.${value}`), value }))
const isTextWidget = computed(() => props.widget.type === 'text')
const isBitmapWidget = computed(() => props.widget.type === 'bitmap')
const supportsFill = computed(() => props.widget.type === 'rect' || props.widget.type === 'circle' || props.widget.type === 'ellipse')
const previewStyle = computed(() => ({
  width: `${Math.max(48, Math.round(props.widget.width * 2))}px`,
  height: isBitmapWidget.value
    ? `${Math.max(56, Math.round(props.widget.height * 2) + 20)}px`
    : `${Math.max(24, Math.round(props.widget.height * 2))}px`,
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
  importSsd1306BitmapFromFile,
  (width, height) => createSsd1306BitmapPlaceholder(width, height).bitmapData,
  () => props.widget.type === 'bitmap',
)

const bitmapError = bitmapWorkflow.bitmapError
const bitmapThreshold = bitmapWorkflow.bitmapThreshold
const bitmapPreviewFrozen = bitmapWorkflow.bitmapPreviewFrozen
const bitmapWidthField = bitmapWorkflow.bitmapWidth
const bitmapHeightField = bitmapWorkflow.bitmapHeight
const bitmapWidget = computed<Ssd1306BitmapWidget | null>(() => (isBitmapWidget.value ? props.widget as Ssd1306BitmapWidget : null))

function updateField<K extends keyof Ssd1306Widget>(key: K, value: Ssd1306Widget[K]): void { emit('update-widget', { [key]: value } as Partial<Ssd1306Widget>) }
function updateKeepAspectRatio(value: boolean): void {
  if (props.widget.type !== 'bitmap') {
    return
  }
  emit('update-widget', { keepAspectRatio: value } as Partial<Ssd1306Widget>)
}
function updateWidgetType(value: string): void { if (['text', 'bitmap', 'rect', 'line', 'circle', 'ellipse'].includes(value)) emit('update-widget', { type: value as Ssd1306Widget['type'] }) }
function updateBindingKind(value: string): void { if (['unbound', 'device', 'metric', 'constant_text'].includes(value)) emit('update-widget', { bindingKind: value as Ssd1306Widget['bindingKind'] }) }
function updateFlag(key: keyof Ssd1306Widget['styleFlags'], value: boolean): void { emit('update-widget', { styleFlags: { ...props.widget.styleFlags, [key]: value } }) }

function updateNumber(key: keyof Pick<Ssd1306Widget, 'x' | 'y' | 'width' | 'height' | 'fontSize' | 'strokeWidth' | 'sourceDeviceId' | 'metricId'>, value: string | number): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) return
  emit('update-widget', { [key]: Math.round(numeric) } as Partial<Ssd1306Widget>)
}

function updateBitmapDimension(key: 'width' | 'height', value: string | number): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  const nextWidth = key === 'width' ? Math.max(1, Math.round(numeric)) : bitmapWidthField.value
  const nextHeight = key === 'height' ? Math.max(1, Math.round(numeric)) : bitmapHeightField.value
  bitmapWorkflow.setBitmapSize(nextWidth, nextHeight)
  emit('update-widget', { width: nextWidth, height: nextHeight } as Partial<Ssd1306Widget>)
}

function onBitmapFileSelected(value: File | File[] | null): void {
  if (value === null || (Array.isArray(value) && value.length === 0)) {
    clearBitmap()
    return
  }
  void bitmapWorkflow.queueBitmapImport(value, props.widget.width, props.widget.height)
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
