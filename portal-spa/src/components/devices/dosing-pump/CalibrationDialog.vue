<template>
  <v-dialog :model-value="modelValue" max-width="480" @update:model-value="$emit('update:modelValue', $event)">
    <v-card>
      <v-card-title>{{ t('device.dialog.dosingPump.calibration.title') }}</v-card-title>
      <v-card-text class="d-flex flex-column ga-4">
        <v-btn-toggle v-model="calibrationMode" mandatory="force" divided density="comfortable">
          <v-btn value="byDose">{{ t('device.dialog.dosingPump.calibration.byDose') }}</v-btn>
          <v-btn value="direct">{{ t('device.dialog.dosingPump.calibration.direct') }}</v-btn>
        </v-btn-toggle>

        <template v-if="calibrationMode === 'byDose'">
          <div class="text-body-2 text-medium-emphasis">
            {{ t('device.dialog.dosingPump.calibration.byDoseHint', { speed: currentSpeed }) }}
          </div>
          <div class="d-flex flex-wrap ga-2">
            <v-btn
              v-for="preset in [5, 10, 50]"
              :key="preset"
              variant="tonal"
              size="small"
              :disabled="dosing"
              @click="testAmountMl = preset"
            >
              {{ preset }} {{ t('device.dialog.dosingPump.ml') }}
            </v-btn>
          </div>
          <v-text-field
            type="number"
            min="0.1"
            step="0.1"
            :label="t('device.dialog.dosingPump.calibration.testAmount')"
            :disabled="dosing"
            v-model.number="testAmountMl"
          />

          <div v-if="dosing" class="d-flex flex-column ga-2">
            <v-progress-linear :model-value="progressPercent" color="primary" height="8" rounded />
            <div class="text-body-2 text-medium-emphasis">
              {{ t('device.dialog.dosingPump.manualDose.progress', { dosed: output?.dosedMl ?? 0, target: output?.dosingTargetMl ?? 0, remaining: output?.dosingRemainingSec ?? 0 }) }}
            </div>
          </div>
          <v-btn v-if="dosing" color="warning" variant="flat" @click="stopTestRun">
            {{ t('device.dialog.dosingPump.manualDose.stop') }}
          </v-btn>
          <v-btn v-else color="primary" variant="tonal" :disabled="!canRunTest" @click="startTestRun">
            {{ t('device.dialog.dosingPump.calibration.run') }}
          </v-btn>

          <v-text-field
            v-if="testRunStarted && !dosing"
            type="number"
            min="0.01"
            step="0.01"
            :label="t('device.dialog.dosingPump.calibration.measured')"
            v-model.number="measuredMl"
          />
          <v-alert v-if="testRunStarted && !dosing && newSpeed > 0" type="info" variant="tonal" density="comfortable">
            {{ t('device.dialog.dosingPump.calibration.result', { current: currentSpeed, next: newSpeed, diff: speedDiffPercent }) }}
          </v-alert>
        </template>

        <template v-else>
          <v-text-field
            type="number"
            min="0.001"
            step="0.001"
            :label="t('device.dialog.dosingPump.calibration.directSpeed')"
            v-model.number="directSpeed"
          />
        </template>
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="$emit('update:modelValue', false)">{{ t('actions.cancel') }}</v-btn>
        <v-btn color="primary" variant="flat" :disabled="!canSave" @click="save">
          {{ t('device.dialog.dosingPump.calibration.save') }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest, DeviceRecord, DosingPumpOutputSnapshot } from '@/api/contracts'
import { calibrationSpeed, doseRunSeconds, speedsAreEquivalent } from '@/models/devices/dosing-pump-math'

const props = defineProps<{
  modelValue: boolean
  device: DeviceRecord
  currentSpeed: number
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  command: [payload: DeviceCommandRequest]
  save: [speedMlPerSec: number]
}>()

const { t } = useI18n()

const calibrationMode = ref<'byDose' | 'direct'>('byDose')
const testAmountMl = ref(10)
const measuredMl = ref(0)
const directSpeed = ref(props.currentSpeed)
const testRunStarted = ref(false)

watch(
  () => props.modelValue,
  open => {
    if (open) {
      testRunStarted.value = false
      measuredMl.value = 0
      directSpeed.value = props.currentSpeed
    }
  },
)

const output = computed(() => (props.device.runtime as { output?: DosingPumpOutputSnapshot }).output)
const dosing = computed(() => output.value?.state === 'dosing')
const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')
const canRunTest = computed(() => isReady.value && Number.isFinite(testAmountMl.value) && testAmountMl.value > 0)

const progressPercent = computed(() => {
  const target = output.value?.dosingTargetMl ?? 0
  const dosed = output.value?.dosedMl ?? 0
  return target > 0 ? Math.min(100, (dosed / target) * 100) : 0
})

// The test run's planned duration is amount / currentSpeed - the same seconds the firmware runs
// the motor for - so newSpeed = measured / plannedSeconds (reference CalibrationDialog math).
const plannedRunSeconds = computed(() => doseRunSeconds(testAmountMl.value, props.currentSpeed))
const newSpeed = computed(() => calibrationSpeed(measuredMl.value, plannedRunSeconds.value))
const speedDiffPercent = computed(() => {
  if (!(props.currentSpeed > 0) || !(newSpeed.value > 0)) {
    return 0
  }
  return Math.round(((newSpeed.value - props.currentSpeed) / props.currentSpeed) * 1000) / 10
})

const canSave = computed(() => {
  if (calibrationMode.value === 'direct') {
    return directSpeed.value > 0 && directSpeed.value <= 65.535 && !speedsAreEquivalent(props.currentSpeed, directSpeed.value)
  }
  return testRunStarted.value && !dosing.value && newSpeed.value > 0 && !speedsAreEquivalent(props.currentSpeed, newSpeed.value)
})

function startTestRun(): void {
  testRunStarted.value = true
  // logging=false: a calibration run must not pollute totals or the dose journal.
  emit('command', { command: 'startDose', amountMl: testAmountMl.value, logging: false })
}

function stopTestRun(): void {
  emit('command', { command: 'stopDose' })
}

function save(): void {
  emit('save', calibrationMode.value === 'direct' ? directSpeed.value : newSpeed.value)
  emit('update:modelValue', false)
}
</script>
