<template>
  <div class="oled-inspector">
    <OledDisplayWidgetPreview :widget="widget" :display-scale="2" class="oled-inspector__preview" :style="previewStyle" />
    <v-alert v-if="bitmapError.length > 0" type="error" variant="tonal" density="compact">
      {{ bitmapError }}
    </v-alert>
    <v-select
      :label="t('device.dialog.oledDisplay.widgetType')"
      :items="widgetTypeItems"
      :model-value="widget.type"
      @update:model-value="updateWidgetType(String($event))"
    />
    <v-row>
      <v-col cols="6"><v-text-field :label="t('device.dialog.oledDisplay.x')" :model-value="widget.x" type="number" min="0" :max="deviceWidth" @update:model-value="updateNumber('x', $event)" /></v-col>
      <v-col cols="6"><v-text-field :label="t('device.dialog.oledDisplay.y')" :model-value="widget.y" type="number" min="0" :max="deviceHeight" @update:model-value="updateNumber('y', $event)" /></v-col>
      <v-col cols="6">
        <v-text-field
          v-if="!isBitmapWidget"
          :label="t('device.dialog.oledDisplay.width')"
          :model-value="widget.width"
          type="number"
          min="1"
          :max="deviceWidth"
          @update:model-value="updateNumber('width', $event)"
        />
        <v-text-field
          v-else
          v-model="bitmapWidthField"
          :label="t('device.dialog.oledDisplay.width')"
          type="number"
          min="1"
          :max="deviceWidth"
        />
      </v-col>
      <v-col cols="6">
        <v-text-field
          v-if="!isBitmapWidget"
          :label="t('device.dialog.oledDisplay.height')"
          :model-value="widget.height"
          type="number"
          min="1"
          :max="deviceHeight"
          @update:model-value="updateNumber('height', $event)"
        />
        <v-text-field
          v-else
          v-model="bitmapHeightField"
          :label="t('device.dialog.oledDisplay.height')"
          type="number"
          min="1"
          :max="deviceHeight"
        />
      </v-col>
      <v-col v-if="isTextWidget" cols="6"><v-text-field :label="t('device.dialog.oledDisplay.fontSize')" :model-value="widget.fontSize" type="number" min="1" :max="8" @update:model-value="updateNumber('fontSize', $event)" /></v-col>
      <v-col cols="6"><v-text-field :label="t('device.dialog.oledDisplay.strokeWidth')" :model-value="widget.strokeWidth" type="number" min="1" :max="32" @update:model-value="updateNumber('strokeWidth', $event)" /></v-col>
    </v-row>
    <v-alert v-if="isBitmapWidget" type="info" variant="tonal" density="compact">
      {{ t('device.dialog.oledDisplay.bitmapSize', { size: `${widget.width} × ${widget.height}` }) }}
    </v-alert>
    <v-alert v-if="isTextWidget" class="oled-inspector__fit" :type="fitInfo.type" variant="tonal" density="compact">
      <div class="oled-inspector__fit-title">{{ fitInfo.title }}</div>
      <div class="text-caption">{{ fitInfo.details }}</div>
    </v-alert>
    <v-text-field
      v-if="isTextWidget"
      :label="t('device.dialog.oledDisplay.text')"
      :model-value="widget.text"
      @update:model-value="updateField('text', String($event))"
    />
    <v-file-input
      v-if="isBitmapWidget"
      accept="image/*"
      :label="t('device.dialog.oledDisplay.bitmapImport')"
      prepend-icon="upload"
      density="comfortable"
      @update:model-value="onBitmapFileSelected"
    />
    <v-slider
      v-if="isBitmapWidget"
      :model-value="bitmapThreshold"
      :min="0"
      :max="255"
      :step="1"
      hide-details
      :label="t('device.dialog.oledDisplay.bitmapThreshold')"
      @update:model-value="updateBitmapThreshold"
    />
    <div v-if="isBitmapWidget" class="oled-inspector__bitmap-actions">
      <v-btn variant="text" @click="clearBitmap">{{ t('device.dialog.oledDisplay.bitmapClear') }}</v-btn>
    </div>
    <v-select
      v-if="isTextWidget"
      :label="t('device.dialog.oledDisplay.bindingKind')"
      :items="bindingKindItems"
      :model-value="widget.bindingKind"
      @update:model-value="updateBindingKind(String($event))"
    />
    <v-row v-if="isTextWidget">
      <v-col cols="6"><v-text-field :label="t('device.dialog.oledDisplay.sourceDeviceId')" :model-value="widget.sourceDeviceId" type="number" min="0" @update:model-value="updateNumber('sourceDeviceId', $event)" /></v-col>
      <v-col cols="6"><v-text-field :label="t('device.dialog.oledDisplay.metricId')" :model-value="widget.metricId" type="number" @update:model-value="updateNumber('metricId', $event)" /></v-col>
    </v-row>
    <v-switch v-if="isTextWidget" :label="t('device.dialog.oledDisplay.autoSize')" :model-value="widget.autoSize" inset @update:model-value="updateField('autoSize', Boolean($event))" />
    <v-switch v-if="supportsFill" :label="t('device.dialog.oledDisplay.filled')" :model-value="widget.styleFlags.filled" inset @update:model-value="updateFlag('filled', Boolean($event))" />
    <v-switch :label="t('device.dialog.oledDisplay.inverted')" :model-value="widget.styleFlags.inverted" inset @update:model-value="updateFlag('inverted', Boolean($event))" />
    <v-switch v-if="isTextWidget" :label="t('device.dialog.oledDisplay.wrap')" :model-value="widget.styleFlags.wrap" inset @update:model-value="updateFlag('wrap', Boolean($event))" />
  </div>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import OledDisplayWidgetPreview from '@/components/devices/oled-display/OledDisplayWidgetPreview.vue'
import {
  createOledDisplayBitmapPlaceholder,
  importOledDisplayBitmapFromFile,
} from '@/components/devices/oled-display/oled-display-bitmap'
import { measureOledDisplayTextWidget } from '@/components/devices/oled-display/oled-display-text-layout'
import type { OledDisplayWidget } from '@/models/devices/oled-display-layout'

const props = defineProps<{
  widget: OledDisplayWidget
  deviceWidth: number
  deviceHeight: number
}>()

const emit = defineEmits<{
  'update-widget': [patch: Partial<OledDisplayWidget>]
}>()

const { t } = useI18n()
const importedBitmapFile = ref<File | null>(null)
const bitmapError = ref('')
const bitmapWidthField = ref(props.widget.width)
const bitmapHeightField = ref(props.widget.height)
const bitmapDraftData = ref<string>(props.widget.type === 'bitmap' ? props.widget.bitmapData : createOledDisplayBitmapPlaceholder().bitmapData)

const widgetTypeItems = ['text', 'bitmap', 'rect', 'line', 'circle', 'ellipse'].map(value => ({ title: t(`device.dialog.oledDisplay.widgetTypes.${value}`), value }))
const bindingKindItems = ['unbound', 'device', 'metric', 'constant_text'].map(value => ({ title: t(`device.dialog.oledDisplay.bindingKinds.${value}`), value }))
const isTextWidget = computed(() => props.widget.type === 'text')
const isBitmapWidget = computed(() => props.widget.type === 'bitmap')
const supportsFill = computed(() => props.widget.type === 'rect' || props.widget.type === 'circle' || props.widget.type === 'ellipse')
const bitmapThreshold = ref(128)
const previewStyle = computed(() => ({
  width: `${Math.max(48, Math.round(props.widget.width * 2))}px`,
  height: `${Math.max(24, Math.round(props.widget.height * 2))}px`,
}))
const fitInfo = computed<{ type: 'info' | 'success' | 'warning'; title: string; details: string }>(() => {
  if (props.widget.type !== 'text') {
    return { type: 'info' as const, title: '', details: '' }
  }
  const measurement = measureOledDisplayTextWidget(props.widget)
  const title = measurement.fits
    ? t('device.dialog.oledDisplay.fits')
    : props.widget.styleFlags.wrap
      ? t('device.dialog.oledDisplay.clipsWrapped')
      : t('device.dialog.oledDisplay.clips')
  const details = props.widget.styleFlags.wrap && measurement.wrappedLines > 1
    ? t('device.dialog.oledDisplay.sizeWrap', { needed: `${measurement.measuredWidth} × ${measurement.measuredHeight}`, box: `${measurement.boxWidth} × ${measurement.boxHeight}`, lines: measurement.wrappedLines })
    : t('device.dialog.oledDisplay.sizeFit', { needed: `${measurement.measuredWidth} × ${measurement.measuredHeight}`, box: `${measurement.boxWidth} × ${measurement.boxHeight}` })
  const type = measurement.fits ? 'success' : 'warning'
  return { type, title, details }
})

function updateField<K extends keyof OledDisplayWidget>(key: K, value: OledDisplayWidget[K]): void {
  emit('update-widget', { [key]: value } as Partial<OledDisplayWidget>)
}

function updateWidgetType(value: string): void {
  if (!['text', 'bitmap', 'rect', 'line', 'circle', 'ellipse'].includes(value)) {
    return
  }
  emit('update-widget', { type: value as OledDisplayWidget['type'] })
}

async function onBitmapFileSelected(value: File | File[] | null): Promise<void> {
  const file = Array.isArray(value) ? value[0] ?? null : value
  if (file === null || !isBitmapWidget.value) {
    return
  }
  importedBitmapFile.value = file
  bitmapError.value = ''
  try {
    const imported = await importOledDisplayBitmapFromFile(file, props.widget.width, props.widget.height, bitmapThreshold.value)
    bitmapDraftData.value = imported.bitmapData
    emit('update-widget', {
      bitmapData: imported.bitmapData,
    } as Partial<OledDisplayWidget>)
  } catch (error) {
    bitmapError.value = error instanceof Error ? error.message : t('device.dialog.oledDisplay.bitmapImportFailed')
  }
}

function updateBitmapThreshold(value: string | number): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  bitmapThreshold.value = Math.max(0, Math.min(255, Math.round(numeric)))
  if (importedBitmapFile.value !== null) {
    void onBitmapFileSelected(importedBitmapFile.value)
  }
}

function clearBitmap(): void {
  importedBitmapFile.value = null
  bitmapError.value = ''
  bitmapDraftData.value = createOledDisplayBitmapPlaceholder(props.widget.width, props.widget.height).bitmapData
  emit('update-widget', { bitmapData: createOledDisplayBitmapPlaceholder(props.widget.width, props.widget.height).bitmapData } as Partial<OledDisplayWidget>)
}

function updateBindingKind(value: string): void {
  if (!['unbound', 'device', 'metric', 'constant_text'].includes(value)) {
    return
  }
  emit('update-widget', { bindingKind: value as OledDisplayWidget['bindingKind'] })
}

function updateNumber(key: keyof Pick<OledDisplayWidget, 'x' | 'y' | 'width' | 'height' | 'fontSize' | 'strokeWidth' | 'sourceDeviceId' | 'metricId'>, value: string | number): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  if (props.widget.type === 'bitmap' && (key === 'width' || key === 'height')) {
    if (key === 'width') {
      bitmapWidthField.value = Math.max(1, Math.round(numeric))
    }
    if (key === 'height') {
      bitmapHeightField.value = Math.max(1, Math.round(numeric))
    }
    emit('update-widget', { [key]: Math.max(1, Math.round(numeric)) } as Partial<OledDisplayWidget>)
    scheduleBitmapResize()
    return
  }
  emit('update-widget', { [key]: Math.round(numeric) } as Partial<OledDisplayWidget>)
}

function scheduleBitmapResize(): void {
  if (!isBitmapWidget.value) {
    return
  }
  if (importedBitmapFile.value === null) {
    return
  }
  void onBitmapFileSelected(importedBitmapFile.value)
}

function updateFlag(key: keyof OledDisplayWidget['styleFlags'], value: boolean): void {
  emit('update-widget', {
    styleFlags: {
      ...props.widget.styleFlags,
      [key]: value,
    },
  })
}

watch(
  () => [props.widget.width, props.widget.height, bitmapThreshold.value],
  async () => {
    if (importedBitmapFile.value === null) {
      return
    }
    await onBitmapFileSelected(importedBitmapFile.value)
  },
)

watch(
  () => [bitmapWidthField.value, bitmapHeightField.value],
  () => {
    if (!isBitmapWidget.value) {
      return
    }
    emit('update-widget', {
      width: Math.max(1, Math.round(bitmapWidthField.value)),
      height: Math.max(1, Math.round(bitmapHeightField.value)),
    } as Partial<OledDisplayWidget>)
    scheduleBitmapResize()
  },
)

watch(
  () => [props.widget.width, props.widget.height, props.widget.type],
  () => {
    bitmapWidthField.value = props.widget.width
    bitmapHeightField.value = props.widget.height
  },
)

watch(
  () => props.widget.type,
  () => {
    bitmapError.value = ''
  },
)
</script>

<style scoped>
.oled-inspector {
  display: grid;
  gap: 12px;
}

.oled-inspector__fit {
  margin-top: -4px;
}

.oled-inspector__fit-title {
  font-weight: 600;
}

.oled-inspector__preview {
  justify-self: start;
  border: 1px solid rgb(var(--v-theme-outline-variant));
  border-radius: 0;
}

</style>
