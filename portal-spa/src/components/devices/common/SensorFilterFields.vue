<template>
  <div>
    <div class="d-flex align-center justify-space-between mb-2">
      <div class="text-body-medium text-medium-emphasis">
        {{ t(titleKey ?? 'device.dialog.sensorFilter.title') }}
      </div>
      <v-btn
        v-if="mode !== 'view'"
        variant="tonal"
        size="small"
        :disabled="busy || !Number.isFinite(currentReading)"
        @click="calibrationOpen = true"
      >
        {{ t('device.dialog.sensorFilter.calibration.open') }}
      </v-btn>
    </div>
    <v-row density="comfortable">
      <v-col cols="12" sm="4">
        <v-text-field
          type="number"
          step="0.01"
          :label="t('device.fields.smoothingWeight')"
          :hint="t('device.dialog.sensorFilter.smoothingWeightHint')"
          :model-value="modelValue.smoothingWeight"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          persistent-hint
          @update:model-value="update('smoothingWeight', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="4">
        <v-text-field
          type="number"
          step="0.01"
          :label="t('device.fields.calibrationFactor')"
          :hint="t('device.dialog.sensorFilter.calibrationFactorHint')"
          :model-value="modelValue.calibrationFactor"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          persistent-hint
          @update:model-value="update('calibrationFactor', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="4">
        <v-text-field
          type="number"
          step="0.01"
          :label="t('device.fields.calibrationOffset')"
          :hint="t('device.dialog.sensorFilter.calibrationOffsetHint')"
          :model-value="modelValue.calibrationOffset"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          persistent-hint
          @update:model-value="update('calibrationOffset', Number($event))"
        />
      </v-col>
    </v-row>

    <SensorCalibrationDialog
      v-model="calibrationOpen"
      :current="modelValue"
      :current-reading="currentReading"
      :reading-unit="readingUnit"
      @apply="applyCalibration"
    />
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useI18n } from 'vue-i18n'
import type { SensorFilterConfig } from '@/models/devices/sensor-filter'
import type { FilterCoefficients } from '@/models/devices/sensor-filter-calibration'
import SensorCalibrationDialog from '@/components/devices/common/SensorCalibrationDialog.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: SensorFilterConfig
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
  titleKey?: string
  currentReading?: number
  readingUnit?: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: SensorFilterConfig]
}>()

const { t } = useI18n()

const { update } = useDraftModel(props, emit)

const calibrationOpen = ref(false)

function applyCalibration(coefficients: FilterCoefficients): void {
  emit('update:modelValue', {
    ...props.modelValue,
    calibrationFactor: coefficients.calibrationFactor,
    calibrationOffset: coefficients.calibrationOffset,
  })
}
</script>
