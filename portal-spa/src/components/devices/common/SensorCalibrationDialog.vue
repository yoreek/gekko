<template>
  <v-dialog :model-value="modelValue" max-width="480" @update:model-value="$emit('update:modelValue', $event)">
    <v-card>
      <v-card-title>{{ t('device.dialog.sensorFilter.calibration.title') }}</v-card-title>
      <v-card-text class="d-flex flex-column ga-4">
        <v-btn-toggle v-model="calibrationMode" mandatory="force" divided density="comfortable">
          <v-btn value="offset">{{ t('device.dialog.sensorFilter.calibration.offsetMode') }}</v-btn>
          <v-btn value="twoPoint">{{ t('device.dialog.sensorFilter.calibration.twoPointMode') }}</v-btn>
        </v-btn-toggle>

        <div class="text-body-2 text-medium-emphasis">
          {{ t('device.dialog.sensorFilter.calibration.stableHint') }}
        </div>

        <v-alert type="info" variant="tonal" density="comfortable">
          {{ t('device.dialog.sensorFilter.calibration.currentReading', { value: readingText }) }}
        </v-alert>

        <div class="text-body-2 text-medium-emphasis">
          {{ t(calibrationMode === 'offset'
            ? 'device.dialog.sensorFilter.calibration.offsetHint'
            : 'device.dialog.sensorFilter.calibration.twoPointHint') }}
        </div>

        <template v-if="calibrationMode === 'offset'">
          <v-text-field
            type="number"
            step="0.1"
            :label="t('device.dialog.sensorFilter.calibration.realTemperature')"
            :suffix="readingUnit"
            v-model.number="offsetReal"
          />
        </template>

        <template v-else>
          <div v-for="(point, index) in twoPoints" :key="index" class="d-flex flex-column ga-2">
            <div class="text-body-medium">{{ t('device.dialog.sensorFilter.calibration.point', { index: index + 1 }) }}</div>
            <div class="d-flex ga-2 align-center">
              <v-text-field
                type="number"
                step="0.1"
                :label="t('device.dialog.sensorFilter.calibration.realTemperature')"
                :suffix="readingUnit"
                v-model.number="point.real"
                hide-details
              />
              <v-btn variant="tonal" size="small" :disabled="!hasReading" @click="capture(index)">
                {{ t('device.dialog.sensorFilter.calibration.capture') }}
              </v-btn>
            </div>
            <div class="text-caption text-medium-emphasis">
              {{
                point.captured
                  ? t('device.dialog.sensorFilter.calibration.captured', { value: formatValue(point.displayed) })
                  : t('device.dialog.sensorFilter.calibration.notCaptured')
              }}
            </div>
          </div>
        </template>

        <v-alert v-if="preview" type="success" variant="tonal" density="comfortable">
          {{ t('device.dialog.sensorFilter.calibration.result', { factor: preview.calibrationFactor, offset: preview.calibrationOffset }) }}
        </v-alert>
        <v-alert v-else-if="showInvalid" type="warning" variant="tonal" density="comfortable">
          {{ t('device.dialog.sensorFilter.calibration.invalid') }}
        </v-alert>
      </v-card-text>
      <v-card-actions>
        <v-spacer />
        <v-btn variant="text" @click="$emit('update:modelValue', false)">{{ t('actions.cancel') }}</v-btn>
        <v-btn color="primary" variant="flat" :disabled="!preview" @click="save">
          {{ t('device.dialog.sensorFilter.calibration.apply') }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { computed, reactive, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import {
  roundCoefficients,
  solveOffsetOnly,
  solveTwoPoint,
  type FilterCoefficients,
} from '@/models/devices/sensor-filter-calibration'

const props = defineProps<{
  modelValue: boolean
  current: FilterCoefficients
  currentReading?: number
  readingUnit?: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  apply: [value: FilterCoefficients]
}>()

const { t } = useI18n()

const calibrationMode = ref<'offset' | 'twoPoint'>('offset')
const offsetReal = ref<number | null>(null)
const twoPoints = reactive([
  { displayed: 0, real: null as number | null, captured: false },
  { displayed: 0, real: null as number | null, captured: false },
])

const hasReading = computed(() => Number.isFinite(props.currentReading))
const readingText = computed(() => (hasReading.value ? formatValue(props.currentReading as number) : '—'))

function formatValue(value: number): string {
  return `${value.toFixed(2)} ${props.readingUnit ?? ''}`.trim()
}

function capture(index: number): void {
  if (!hasReading.value) return
  twoPoints[index].displayed = props.currentReading as number
  twoPoints[index].captured = true
}

const preview = computed<FilterCoefficients | null>(() => {
  if (calibrationMode.value === 'offset') {
    if (!hasReading.value || !Number.isFinite(offsetReal.value)) return null
    return roundCoefficients(
      solveOffsetOnly({ displayed: props.currentReading as number, real: offsetReal.value as number }, props.current),
    )
  }
  const [p1, p2] = twoPoints
  if (!p1.captured || !p2.captured || !Number.isFinite(p1.real) || !Number.isFinite(p2.real)) return null
  const solved = solveTwoPoint(
    { displayed: p1.displayed, real: p1.real as number },
    { displayed: p2.displayed, real: p2.real as number },
    props.current,
  )
  return solved ? roundCoefficients(solved) : null
})

// Only warn once the user has entered enough to expect a result but the points are degenerate.
const showInvalid = computed(() => {
  if (preview.value) return false
  if (calibrationMode.value === 'twoPoint') {
    const [p1, p2] = twoPoints
    return p1.captured && p2.captured && Number.isFinite(p1.real) && Number.isFinite(p2.real)
  }
  return false
})

function resetState(): void {
  calibrationMode.value = 'offset'
  offsetReal.value = null
  twoPoints[0].captured = false
  twoPoints[0].real = null
  twoPoints[1].captured = false
  twoPoints[1].real = null
}

watch(
  () => props.modelValue,
  open => {
    if (open) resetState()
  },
)

function save(): void {
  if (!preview.value) return
  emit('apply', preview.value)
  emit('update:modelValue', false)
}
</script>
