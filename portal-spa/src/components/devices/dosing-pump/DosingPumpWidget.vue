<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="pump" />
    </template>
    <div class="d-flex ga-2 w-100">
      <div class="d-flex flex-column ga-1 flex-grow-1">
        <div class="d-flex align-center ga-1">
          <v-chip v-if="dosing" size="small" variant="tonal" color="primary">
            {{ t('device.dialog.dosingPump.stateDosing', { remaining: output?.dosingRemainingSec ?? 0 }) }}
          </v-chip>
          <v-chip v-else size="small" variant="tonal" :color="autoMode ? 'info' : 'secondary'">
            {{ t(autoMode ? 'device.dialog.dosingPump.modeAuto' : 'device.dialog.dosingPump.modeManual') }}
          </v-chip>
        </div>
        <div class="text-body-2 text-medium-emphasis">
          {{ t('device.dialog.dosingPump.todayShort', { dosed: todayDosedMl, target: todayTargetMl }) }}
        </div>
        <v-progress-linear :model-value="todayPercent" color="primary" height="6" rounded />
        <div class="text-label-small text-medium-emphasis">{{ scheduleSummary }}</div>
      </div>
      <div class="d-flex flex-column align-center ga-1 flex-shrink-0">
        <div class="d-flex align-center ga-1">
          <ContainerTank :percent="containerPercent" :color="containerColor" />
          <span class="text-body-2" :class="containerStatus !== 'normal' ? `text-${containerColor}` : undefined">
            {{ Math.round(containerPercent) }}%
          </span>
        </div>
        <span class="text-label-small text-medium-emphasis text-no-wrap">
          {{ t('device.dialog.dosingPump.containerShort', { current: container?.currentMl ?? 0, capacity: container?.capacityMl ?? 0 }) }}
        </span>
      </div>
    </div>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-3 pa-2">
    <div class="d-flex flex-wrap align-center ga-2">
      <v-btn-toggle :model-value="autoMode ? 'auto' : 'manual'" mandatory="force" divided density="comfortable" :disabled="!isReady" @update:model-value="setMode($event as string)">
        <v-btn value="manual">{{ t('device.dialog.dosingPump.modeManual') }}</v-btn>
        <v-btn value="auto">{{ t('device.dialog.dosingPump.modeAuto') }}</v-btn>
      </v-btn-toggle>
      <v-chip v-if="dosing" variant="tonal" color="primary">
        {{ t('device.dialog.dosingPump.stateDosing', { remaining: output?.dosingRemainingSec ?? 0 }) }}
      </v-chip>
    </div>

    <div v-if="dosing" class="d-flex flex-column ga-1">
      <v-progress-linear :model-value="dosingPercent" color="primary" height="8" rounded />
      <div class="text-body-2 text-medium-emphasis">
        {{ t('device.dialog.dosingPump.manualDose.progress', { dosed: output?.dosedMl ?? 0, target: output?.dosingTargetMl ?? 0, remaining: output?.dosingRemainingSec ?? 0 }) }}
      </div>
    </div>

    <v-sheet border rounded class="pa-3 d-flex flex-column ga-1">
      <div class="d-flex justify-space-between text-body-2">
        <span>{{ t('device.dialog.dosingPump.today') }}</span>
        <span>{{ t('device.dialog.dosingPump.todayShort', { dosed: todayDosedMl, target: todayTargetMl }) }}</span>
      </div>
      <v-progress-linear :model-value="todayPercent" color="primary" height="8" rounded />
    </v-sheet>

    <v-sheet border rounded class="pa-3 d-flex align-center ga-3">
      <ContainerTank :percent="containerPercent" :color="containerColor" :width="34" :height="48" />
      <div class="d-flex flex-column">
        <span class="text-body-2">{{ t('device.dialog.dosingPump.container.title') }} · {{ Math.round(containerPercent) }}%</span>
        <span class="text-body-2 text-medium-emphasis">
          {{ t('device.dialog.dosingPump.containerShort', { current: container?.currentMl ?? 0, capacity: container?.capacityMl ?? 0 }) }}
          <template v-if="output?.daysLeft !== undefined"> · {{ t('device.dialog.dosingPump.daysLeft', { days: output?.daysLeft }) }}</template>
        </span>
      </div>
    </v-sheet>

    <v-sheet border rounded class="pa-3 d-flex flex-column ga-2">
      <div class="text-label-small text-medium-emphasis">{{ t('device.dialog.dosingPump.schedule.title') }}</div>
      <div v-if="output?.nextDoseAt" class="d-flex align-center ga-2 text-body-2 text-primary">
        <v-icon icon="time" size="small" />
        <span>{{ t('device.dialog.dosingPump.nextDose', { at: formatDoseEpoch(output.nextDoseAt), amount: output.nextDoseAmountMl ?? 0 }) }}</span>
      </div>
      <div v-if="output?.lastDose?.at" class="d-flex align-center ga-2 text-body-2 text-medium-emphasis">
        <v-icon icon="check-circle" size="small" />
        <span>{{ t('device.dialog.dosingPump.lastDose', { at: formatDoseEpoch(output.lastDose.at), amount: output.lastDose.amountMl ?? 0 }) }}</span>
      </div>
      <template v-if="scheduleDoses.length > 0">
        <v-divider />
        <div class="d-flex flex-wrap ga-1">
          <v-chip
            v-for="(dose, index) in scheduleDoses"
            :key="index"
            size="small"
            :variant="skipNext[index] ? 'outlined' : 'tonal'"
            :color="skipNext[index] ? 'warning' : undefined"
            :disabled="!isReady"
            @click="toggleSkip(index)"
          >
            {{ dose.time }} · {{ dose.amountMl }} {{ t('device.dialog.dosingPump.ml') }}
            <template v-if="skipNext[index]"> · {{ t('device.dialog.dosingPump.skipped') }}</template>
          </v-chip>
        </div>
        <div class="text-label-small text-medium-emphasis">{{ t('device.dialog.dosingPump.skipHint') }}</div>
      </template>
    </v-sheet>

    <div class="d-flex flex-wrap ga-2">
      <v-btn variant="tonal" size="small" :disabled="!isReady" @click="manualDoseOpen = true">
        {{ t('device.dialog.dosingPump.manualDose.title') }}
      </v-btn>
      <v-btn variant="tonal" size="small" :disabled="!isReady" @click="calibrationOpen = true">
        {{ t('device.dialog.dosingPump.calibration.title') }}
      </v-btn>
      <v-btn variant="tonal" size="small" @click="containerOpen = true">
        {{ t('device.dialog.dosingPump.container.title') }}
      </v-btn>
      <v-btn variant="tonal" size="small" @click="historyOpen = true">
        {{ t('device.dialog.dosingPump.history.title') }}
      </v-btn>
    </div>

    <ManualDoseDialog v-model="manualDoseOpen" :device="device" @command="emit('command', $event)" />
    <CalibrationDialog
      v-model="calibrationOpen"
      :device="device"
      :current-speed="configDraft.dosingSpeedMlPerSec"
      @command="emit('command', $event)"
      @save="saveSpeed"
    />
    <ContainerEditDialog
      v-model="containerOpen"
      :device="device"
      :container="configDraft.container"
      @command="emit('command', $event)"
      @save-config="saveContainerConfig"
    />
    <DoseHistoryDialog v-model="historyOpen" :device="device" />
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import type { BaseDeviceRuntime, DeviceCommandRequest, DeviceRecord, DosingPumpOutputSnapshot } from '@/api/contracts'
import { DosingPumpDevice, type DosingPumpContainerDraft } from '@/models/devices/dosing-pump'
import { resolveDeviceModelByTypeName } from '@/models/devices/device-model-factory'
import { formatLocalFlavoredEpoch } from '@/models/devices/dosing-pump-math'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import ContainerTank from './ContainerTank.vue'
import ManualDoseDialog from './ManualDoseDialog.vue'
import CalibrationDialog from './CalibrationDialog.vue'
import ContainerEditDialog from './ContainerEditDialog.vue'
import DoseHistoryDialog from './DoseHistoryDialog.vue'

interface DosingPumpRuntime extends BaseDeviceRuntime {
  output?: DosingPumpOutputSnapshot
}

const props = withDefaults(
  defineProps<{
    device: DeviceRecord<any, DosingPumpRuntime>
    editable?: boolean
    dense?: boolean
  }>(),
  {
    dense: true,
  },
)

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t, locale } = useI18n()

function formatDoseEpoch(epochSeconds: number): string {
  return formatLocalFlavoredEpoch(epochSeconds, locale.value === 'ru' ? 'ru-RU' : 'en-US')
}

const manualDoseOpen = ref(false)
const calibrationOpen = ref(false)
const containerOpen = ref(false)
const historyOpen = ref(false)

const model = computed(() => resolveDeviceModelByTypeName(DosingPumpDevice.TYPE_NAME) as DosingPumpDevice)
const configDraft = computed(() => model.value.createEditDraft(props.device))

const output = computed(() => props.device.runtime.output)
const dosing = computed(() => output.value?.state === 'dosing')
const autoMode = computed(() => output.value?.autoMode === true)
const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready' && !props.editable)

const todayDosedMl = computed(() => output.value?.todayDosedMl ?? 0)
const todayTargetMl = computed(() => output.value?.todayTargetMl ?? 0)
const todayPercent = computed(() => (todayTargetMl.value > 0 ? Math.min(100, (todayDosedMl.value / todayTargetMl.value) * 100) : 0))

const container = computed(() => output.value?.container)
const containerPercent = computed(() => container.value?.percent ?? 0)
const containerStatus = computed(() => container.value?.status ?? 'normal')
const containerColor = computed(() =>
  containerStatus.value === 'critical' ? 'error' : containerStatus.value === 'warning' ? 'warning' : 'primary',
)

const dosingPercent = computed(() => {
  const target = output.value?.dosingTargetMl ?? 0
  const dosed = output.value?.dosedMl ?? 0
  return target > 0 ? Math.min(100, (dosed / target) * 100) : 0
})

const scheduleDoses = computed(() => configDraft.value.schedule.doses)
const scheduleSummary = computed(() => {
  const schedule = configDraft.value.schedule
  if (schedule.mode === 'weekly') {
    return [...schedule.daysOfWeek]
      .sort((a, b) => a - b)
      .map((day) => t(`device.dialog.dosingPump.schedule.weekday.${day}`))
      .join(', ')
  }
  return t('device.dialog.dosingPump.schedule.everyDaysSummary', { n: schedule.everyDays })
})
const skipNext = computed(() => {
  const flags = output.value?.skipNext ?? []
  return scheduleDoses.value.map((_, index) => flags[index] === true)
})

function setMode(mode: string): void {
  emit('command', { command: 'setMode', mode: mode === 'auto' ? 'auto' : 'manual' })
}

function toggleSkip(index: number): void {
  emit('command', { command: 'skipNext', doseIndex: index, skip: !skipNext.value[index] })
}

// Config writes go through the model's quick-update path so diffing/deps handling stays
// identical to the edit dialog (never hand-build an updateConfig payload here).
function saveSpeed(speedMlPerSec: number): void {
  for (const command of model.value.buildQuickUpdateCommands(props.device, { dosingSpeedMlPerSec: speedMlPerSec })) {
    emit('command', command)
  }
}

function saveContainerConfig(containerDraft: DosingPumpContainerDraft): void {
  for (const command of model.value.buildQuickUpdateCommands(props.device, { container: containerDraft })) {
    emit('command', command)
  }
}
</script>
