<template>
  <div>
    <div class="text-body-medium text-medium-emphasis mb-2">
      {{ t('device.dialog.sensorFilter.title') }}
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
  </div>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import type { SensorFilterConfig } from '@/models/devices/sensor-filter'

const props = defineProps<{
  modelValue: SensorFilterConfig
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: SensorFilterConfig]
}>()

const { t } = useI18n()

function update<K extends keyof SensorFilterConfig>(key: K, value: SensorFilterConfig[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value })
}
</script>
