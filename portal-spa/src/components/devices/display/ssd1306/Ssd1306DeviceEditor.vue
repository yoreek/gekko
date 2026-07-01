<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <I2cAddressPicker
        :model-value="currentValue.i2cAddress"
        :bus-device-id="currentValue.i2cBusDeviceId"
        :input-label="t('device.fields.display.i2cAddress')"
        :select-label="t('device.dialog.scanResults')"
        :hint="t('device.dialog.i2cAddressHint')"
        :busy="busy"
        :no-dependency-text="t('device.dialog.i2cAddressNoDependency')"
        :scan-action-text="t('device.dialog.i2cScanAction')"
        :scan-in-progress-text="t('device.dialog.i2cScanInProgress')"
        :scan-empty-text="t('device.dialog.i2cScanEmpty')"
        :scan-truncated-text="t('device.dialog.i2cScanTruncated')"
        :error-fallback-text="t('device.dialog.unknownError')"
        @update:model-value="updateNumber('i2cAddress', $event)"
      />

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.display.orientation')"
            :items="orientationItems"
            :model-value="currentValue.rotation"
            :disabled="busy"
            density="compact"
            variant="outlined"
            hide-details
            @update:model-value="updateNumber('rotation', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field v-select-on-focus :label="t('device.fields.display.width')" :model-value="currentValue.width" :disabled="busy" inputmode="numeric" type="number" min="1" @update:model-value="updateNumber('width', $event)" />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field v-select-on-focus :label="t('device.fields.display.height')" :model-value="currentValue.height" :disabled="busy" inputmode="numeric" type="number" min="1" @update:model-value="updateNumber('height', $event)" />
        </v-col>
      </v-row>
    </section>
    <section class="device-type-section">
      <div class="text-subtitle-2">{{ t('device.fields.display.layout') }}</div>
      <div class="text-body-2">{{ t('device.dialog.ssd1306.layoutHint') }}</div>
      <Ssd1306LayoutPreview
        :layout="currentValue.layout"
        :display="ssd1306Display"
        :device-width="effectiveSize.effectiveWidth"
        :device-height="effectiveSize.effectiveHeight"
        :metric-catalog="metricCatalog"
      />
      <div class="d-flex justify-end">
        <v-btn variant="text" color="primary" :disabled="busy" @click="emit('design-display')">
          <v-icon class="me-1" icon="design-display" />
          {{ t('device.dialog.ssd1306Display.designDisplay') }}
        </v-btn>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'
import I2cAddressPicker from '@/components/devices/common/I2cAddressPicker.vue'
import Ssd1306LayoutPreview from '@/components/devices/display/ssd1306/Ssd1306LayoutPreview.vue'
import { useMetricPlaceholderCatalog } from '@/composables/display/useMetricPlaceholderCatalog'
import { ssd1306Display } from '@/models/devices/display/display'
import { resolveDisplayEffectiveSize } from '@/models/devices/display/orientation'
import { Ssd1306Device, type Ssd1306ConfigDraft, type Ssd1306CreateDraft } from '@/models/devices/ssd1306/device'

type FormValue = Ssd1306CreateDraft | Ssd1306ConfigDraft

const props = defineProps<{
  modelValue?: FormValue
  device?: DeviceRecord
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: FormValue]
  'design-display': []
}>()

const { t } = useI18n()
const { metricCatalog, refreshMetricCatalog } = useMetricPlaceholderCatalog()

const fallbackValue: Ssd1306CreateDraft = {
  ...Ssd1306Device.defaultConfig(),
  typeName: 'ssd1306',
}

const currentValue = computed<FormValue>(() => props.modelValue ?? fallbackValue)
const effectiveSize = computed(() => resolveDisplayEffectiveSize(currentValue.value.width, currentValue.value.height, currentValue.value.rotation))
const orientationItems = computed(() => {
  const isWidePanel = currentValue.value.width >= currentValue.value.height
  const naturalLabel = isWidePanel ? t('device.fields.display.orientationLandscape') : t('device.fields.display.orientationPortrait')
  const rotatedLabel = isWidePanel ? t('device.fields.display.orientationPortrait') : t('device.fields.display.orientationLandscape')
  return [
    { title: naturalLabel, value: 0 },
    { title: rotatedLabel, value: 1 },
  ]
})

onMounted(() => {
  void refreshMetricCatalog()
})

function updateNumber(key: keyof Pick<Ssd1306CreateDraft, 'i2cBusDeviceId' | 'i2cAddress' | 'rotation' | 'width' | 'height'>, value: string | number): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) return
  emit('update:modelValue', { ...fallbackValue, ...(currentValue.value as Partial<Ssd1306CreateDraft>), [key]: numeric } as FormValue)
}
</script>
