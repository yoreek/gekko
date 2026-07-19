<template>
  <v-dialog :model-value="modelValue" max-width="480" @update:model-value="$emit('update:modelValue', $event)">
    <v-card>
      <v-card-title>{{ t('device.dialog.dosingPump.generator.title') }}</v-card-title>
      <v-card-text>
        <v-row density="comfortable">
          <v-col cols="6">
            <v-text-field
              type="time"
              :label="t('device.dialog.dosingPump.generator.startTime')"
              v-model="startTime"
            />
          </v-col>
          <v-col cols="6">
            <v-text-field
              type="time"
              :label="t('device.dialog.dosingPump.generator.endTime')"
              v-model="endTime"
            />
          </v-col>
          <v-col cols="6">
            <v-text-field
              type="number"
              min="0.1"
              step="0.1"
              :label="t('device.dialog.dosingPump.generator.totalAmount')"
              v-model.number="totalAmount"
            />
          </v-col>
          <v-col cols="6">
            <v-text-field
              type="number"
              min="1"
              :max="DOSING_PUMP_MAX_DOSES"
              :label="t('device.dialog.dosingPump.generator.doseCount')"
              v-model.number="doseCount"
            />
          </v-col>
        </v-row>

        <v-alert v-if="generated === null" type="warning" variant="tonal" density="comfortable">
          {{ t('device.dialog.dosingPump.generator.invalid') }}
        </v-alert>
        <div v-else class="text-body-2 text-medium-emphasis">
          {{ t('device.dialog.dosingPump.generator.preview', { count: generated.length, first: generated[0]?.amountMl, last: generated[generated.length - 1]?.amountMl }) }}
        </div>
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="$emit('update:modelValue', false)">{{ t('actions.cancel') }}</v-btn>
        <v-btn color="primary" variant="flat" :disabled="generated === null" @click="applyGenerated">
          {{ t('device.dialog.dosingPump.generator.apply') }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import { DOSING_PUMP_MAX_DOSES, generateDoses, type GeneratedDose } from '@/models/devices/dosing-pump-math'

defineProps<{
  modelValue: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  generate: [doses: GeneratedDose[]]
}>()

const { t } = useI18n()

const startTime = ref('09:00')
const endTime = ref('21:00')
const totalAmount = ref(30)
const doseCount = ref(4)

const generated = computed(() => generateDoses(startTime.value, endTime.value, totalAmount.value, doseCount.value))

function applyGenerated(): void {
  if (generated.value === null) {
    return
  }
  emit('generate', generated.value)
  emit('update:modelValue', false)
}
</script>
