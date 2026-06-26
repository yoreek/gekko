<template>
  <div class="oled-inspector">
    <OledDisplayWidgetPreview :widget="widget" :display-scale="2" class="oled-inspector__preview" :style="previewStyle" />
    <v-select
      :label="t('device.dialog.oledDisplay.widgetType')"
      :items="widgetTypeItems"
      :model-value="widget.type"
      @update:model-value="updateWidgetType(String($event))"
    />
    <v-row>
      <v-col cols="6"><v-text-field :label="t('device.dialog.oledDisplay.x')" :model-value="widget.x" type="number" min="0" :max="deviceWidth" @update:model-value="updateNumber('x', $event)" /></v-col>
      <v-col cols="6"><v-text-field :label="t('device.dialog.oledDisplay.y')" :model-value="widget.y" type="number" min="0" :max="deviceHeight" @update:model-value="updateNumber('y', $event)" /></v-col>
      <v-col cols="6"><v-text-field :label="t('device.dialog.oledDisplay.width')" :model-value="widget.width" type="number" min="1" :max="widget.type === 'circle' ? Math.min(deviceWidth, deviceHeight) : deviceWidth" :disabled="widget.type === 'text' && widget.autoSize" @update:model-value="updateNumber('width', $event)" /></v-col>
      <v-col cols="6"><v-text-field :label="t('device.dialog.oledDisplay.height')" :model-value="widget.height" type="number" min="1" :max="deviceHeight" :disabled="(widget.type === 'text' && widget.autoSize) || widget.type === 'circle'" @update:model-value="updateNumber('height', $event)" /></v-col>
      <v-col v-if="isTextWidget" cols="6"><v-text-field :label="t('device.dialog.oledDisplay.fontSize')" :model-value="widget.fontSize" type="number" min="1" :max="8" @update:model-value="updateNumber('fontSize', $event)" /></v-col>
      <v-col cols="6"><v-text-field :label="t('device.dialog.oledDisplay.strokeWidth')" :model-value="widget.strokeWidth" type="number" min="1" :max="32" @update:model-value="updateNumber('strokeWidth', $event)" /></v-col>
    </v-row>
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
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import OledDisplayWidgetPreview from '@/components/devices/oled-display/OledDisplayWidgetPreview.vue'
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

const widgetTypeItems = ['text', 'rect', 'line', 'circle', 'ellipse'].map(value => ({ title: t(`device.dialog.oledDisplay.widgetTypes.${value}`), value }))
const bindingKindItems = ['unbound', 'device', 'metric', 'constant_text'].map(value => ({ title: t(`device.dialog.oledDisplay.bindingKinds.${value}`), value }))
const isTextWidget = computed(() => props.widget.type === 'text')
const supportsFill = computed(() => props.widget.type === 'rect' || props.widget.type === 'circle' || props.widget.type === 'ellipse')
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
  if (!['text', 'rect', 'line', 'circle', 'ellipse'].includes(value)) {
    return
  }
  emit('update-widget', { type: value as OledDisplayWidget['type'] })
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
  emit('update-widget', { [key]: Math.round(numeric) } as Partial<OledDisplayWidget>)
}

function updateFlag(key: keyof OledDisplayWidget['styleFlags'], value: boolean): void {
  emit('update-widget', {
    styleFlags: {
      ...props.widget.styleFlags,
      [key]: value,
    },
  })
}
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
