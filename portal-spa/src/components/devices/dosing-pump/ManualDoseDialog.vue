<template>
  <v-dialog :model-value="modelValue" max-width="440" @update:model-value="$emit('update:modelValue', $event)">
    <v-card>
      <v-card-title>{{ t('device.dialog.dosingPump.manualDose.title') }}</v-card-title>
      <v-card-text class="d-flex flex-column ga-4">
        <div class="d-flex flex-wrap ga-2">
          <v-btn
            v-for="preset in [5, 10, 50]"
            :key="preset"
            variant="tonal"
            size="small"
            :disabled="dosing"
            @click="amountMl = preset"
          >
            {{ preset }} {{ t('device.dialog.dosingPump.ml') }}
          </v-btn>
        </div>
        <v-text-field
          type="number"
          min="0.1"
          step="0.1"
          :label="t('device.dialog.dosingPump.manualDose.amount')"
          :disabled="dosing"
          v-model.number="amountMl"
        />

        <div v-if="dosing" class="d-flex flex-column ga-2">
          <v-progress-linear :model-value="progressPercent" color="primary" height="8" rounded />
          <div class="text-body-2 text-medium-emphasis">
            {{ t('device.dialog.dosingPump.manualDose.progress', { dosed: output?.dosedMl ?? 0, target: output?.dosingTargetMl ?? 0, remaining: output?.dosingRemainingSec ?? 0 }) }}
          </div>
        </div>
        <div v-else-if="lastRunDosedMl > 0" class="text-body-2 text-medium-emphasis">
          {{ t('device.dialog.dosingPump.manualDose.lastResult', { amount: lastRunDosedMl }) }}
        </div>
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="$emit('update:modelValue', false)">{{ t('actions.close') }}</v-btn>
        <v-btn v-if="dosing" color="warning" variant="flat" @click="stop">
          {{ t('device.dialog.dosingPump.manualDose.stop') }}
        </v-btn>
        <v-btn v-else color="primary" variant="flat" :disabled="!canStart" @click="start">
          {{ t('device.dialog.dosingPump.manualDose.start') }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest, DeviceRecord, DosingPumpOutputSnapshot } from '@/api/contracts'

const props = defineProps<{
  modelValue: boolean
  device: DeviceRecord
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()

const amountMl = ref(10)

const output = computed(() => (props.device.runtime as { output?: DosingPumpOutputSnapshot }).output)
// Live progress comes straight from the device's WS runtime pushes (the firmware marks itself
// runtime-dirty once per second while dosing) - no local timer needed.
const dosing = computed(() => output.value?.state === 'dosing')
const lastRunDosedMl = computed(() => output.value?.lastRunDosedMl ?? 0)
const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')
const canStart = computed(() => isReady.value && Number.isFinite(amountMl.value) && amountMl.value > 0 && amountMl.value <= 655.35)

const progressPercent = computed(() => {
  const target = output.value?.dosingTargetMl ?? 0
  const dosed = output.value?.dosedMl ?? 0
  return target > 0 ? Math.min(100, (dosed / target) * 100) : 0
})

function start(): void {
  emit('command', { command: 'startDose', amountMl: amountMl.value, logging: true })
}

function stop(): void {
  emit('command', { command: 'stopDose' })
}
</script>
