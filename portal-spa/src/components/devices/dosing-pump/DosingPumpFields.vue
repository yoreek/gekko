<template>
  <div class="d-flex flex-column ga-4">
    <v-alert v-if="switchItems.length === 0" type="warning" variant="tonal">
      {{ t('device.dialog.dosingPump.noSwitch') }}
    </v-alert>

    <v-row>
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.pumpSwitch')"
          :items="switchItems"
          :model-value="modelValue.pumpSwitchDeviceId"
          :readonly="mode === 'view'"
          :disabled="(busy && mode !== 'view') || switchItems.length === 0"
          @update:model-value="update('pumpSwitchDeviceId', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          min="0.001"
          step="0.001"
          :label="t('device.fields.dosingSpeed')"
          :hint="t('device.dialog.dosingPump.speedHint')"
          persistent-hint
          :model-value="modelValue.dosingSpeedMlPerSec"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('dosingSpeedMlPerSec', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.levelSensor')"
          :items="sensorItems"
          :model-value="modelValue.levelSensorDeviceId"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('levelSensorDeviceId', Number($event))"
        />
      </v-col>
      <v-col v-if="modelValue.levelSensorDeviceId > 0" cols="12" sm="6">
        <v-switch
          :label="t('device.fields.conditionInvert')"
          :model-value="modelValue.levelSensorInvert"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          density="comfortable"
          hide-details
          inset
          @update:model-value="update('levelSensorInvert', Boolean($event))"
        />
      </v-col>
    </v-row>

    <div class="text-label-small text-medium-emphasis">{{ t('device.dialog.dosingPump.container.title') }}</div>
    <v-row>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          min="1"
          max="65535"
          :label="t('device.dialog.dosingPump.container.capacity')"
          :model-value="modelValue.container.capacityMl"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="updateContainer({ capacityMl: Number($event) })"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          min="0"
          max="100"
          :label="t('device.dialog.dosingPump.container.threshold')"
          :model-value="modelValue.container.thresholdPercent"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="updateContainer({ thresholdPercent: Number($event) })"
        />
      </v-col>
      <v-col cols="12">
        <v-switch
          :label="t('device.dialog.dosingPump.container.blockAutoWhenEmpty')"
          :model-value="modelValue.container.blockAutoWhenEmpty"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          density="comfortable"
          hide-details
          inset
          @update:model-value="updateContainer({ blockAutoWhenEmpty: Boolean($event) })"
        />
      </v-col>
    </v-row>

    <div class="text-label-small text-medium-emphasis">{{ t('device.dialog.dosingPump.schedule.title') }}</div>
    <div class="d-flex flex-wrap align-center ga-3">
      <v-btn-toggle
        :model-value="modelValue.schedule.mode"
        mandatory="force"
        divided
        density="comfortable"
        :disabled="(busy && mode !== 'view') || mode === 'view'"
        @update:model-value="updateSchedule({ mode: $event as DosingScheduleMode })"
      >
        <v-btn value="daily">{{ t('device.dialog.dosingPump.schedule.daily') }}</v-btn>
        <v-btn value="weekly">{{ t('device.dialog.dosingPump.schedule.weekly') }}</v-btn>
      </v-btn-toggle>

      <v-text-field
        v-if="modelValue.schedule.mode === 'daily'"
        type="number"
        min="1"
        max="30"
        class="flex-grow-1"
        :label="t('device.dialog.dosingPump.schedule.everyDays')"
        :model-value="modelValue.schedule.everyDays"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        density="comfortable"
        hide-details
        @update:model-value="updateEveryDays(Number($event))"
      />
    </div>

    <v-row v-if="modelValue.schedule.mode === 'weekly'">
      <v-col v-for="day in 7" :key="day - 1" cols="auto">
        <v-checkbox
          :label="t(`device.dialog.dosingPump.schedule.weekday.${day - 1}`)"
          :model-value="modelValue.schedule.daysOfWeek"
          :value="day - 1"
          :disabled="(busy && mode !== 'view') || mode === 'view'"
          density="compact"
          hide-details
          @update:model-value="updateSchedule({ daysOfWeek: ($event as number[]).sort((a, b) => a - b) })"
        />
      </v-col>
      <v-col cols="auto" class="d-flex align-center">
        <v-btn
          size="small"
          variant="text"
          :disabled="(busy && mode !== 'view') || mode === 'view'"
          @click="toggleAllDoseDays"
        >
          {{ modelValue.schedule.daysOfWeek.length === 7 ? t('device.dialog.schedule.deselectAllDays') : t('device.dialog.schedule.selectAllDays') }}
        </v-btn>
      </v-col>
    </v-row>

    <v-alert v-if="modelValue.schedule.doses.length === 0" type="info" variant="tonal" density="comfortable">
      {{ t('device.dialog.dosingPump.schedule.noDoses') }}
    </v-alert>

    <v-row v-for="(dose, index) in modelValue.schedule.doses" :key="index" align="center" density="comfortable">
      <v-col cols="5" sm="4">
        <v-text-field
          type="time"
          :label="t('device.dialog.dosingPump.schedule.time')"
          :model-value="dose.time"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          density="comfortable"
          hide-details
          @update:model-value="updateDose(index, { time: String($event) })"
        />
      </v-col>
      <v-col cols="5" sm="4">
        <v-text-field
          type="number"
          min="0.01"
          step="0.1"
          :label="t('device.dialog.dosingPump.schedule.amount')"
          :model-value="dose.amountMl"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          density="comfortable"
          hide-details
          @update:model-value="updateDose(index, { amountMl: Number($event) })"
        />
      </v-col>
      <v-col cols="2" class="d-flex justify-end">
        <v-btn
          v-if="mode !== 'view'"
          icon="trash"
          variant="text"
          density="comfortable"
          :disabled="busy"
          :aria-label="t('device.dialog.dosingPump.schedule.removeDose')"
          @click="removeDose(index)"
        />
      </v-col>
    </v-row>

    <div class="d-flex flex-wrap align-center ga-2">
      <v-btn
        v-if="mode !== 'view'"
        variant="tonal"
        prepend-icon="plus"
        :disabled="busy || modelValue.schedule.doses.length >= DOSING_PUMP_MAX_DOSES"
        @click="addDose"
      >
        {{ t('device.dialog.dosingPump.schedule.addDose') }}
      </v-btn>
      <v-btn v-if="mode !== 'view'" variant="tonal" :disabled="busy" @click="generatorOpen = true">
        {{ t('device.dialog.dosingPump.schedule.generate') }}
      </v-btn>
      <v-spacer />
      <div class="text-body-2 text-medium-emphasis">
        {{ t('device.dialog.dosingPump.schedule.total', { amount: totalAmount }) }}
      </div>
    </div>

    <DoseGeneratorDialog v-model="generatorOpen" @generate="applyGeneratedDoses" />

    <template v-if="device">
      <v-divider />
      <div class="text-label-small text-medium-emphasis">{{ t('device.dialog.quickCommands') }}</div>
      <!-- The same operational panel the dashboard MoreInfo dialog shows (mode toggle, live
           progress, manual dose / calibration / container / history dialogs). -->
      <DosingPumpWidget :device="device" :dense="false" @command="emit('command', $event)" />
    </template>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import { dependencyOptionsForRole, devicesForDependencyRole } from '@/models/devices/device-model-factory'
import type {
  DosingPumpConfigDraft,
  DosingPumpContainerDraft,
  DosingPumpDoseDraft,
  DosingPumpScheduleDraft,
  DosingScheduleMode,
} from '@/models/devices/dosing-pump'
import { DOSING_PUMP_MAX_DOSES, todayLocalDayNumber, totalScheduleAmount, type GeneratedDose } from '@/models/devices/dosing-pump-math'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useDraftModel } from '@/composables/useDraftModel'
import DoseGeneratorDialog from './DoseGeneratorDialog.vue'
import DosingPumpWidget from './DosingPumpWidget.vue'

const props = defineProps<{
  modelValue: DosingPumpConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: DosingPumpConfigDraft]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const generatorOpen = ref(false)

const switchItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'switch'))
const sensorItems = computed(() => [
  { title: t('device.dialog.dosingPump.noLevelSensor'), value: 0 },
  ...devicesForDependencyRole(deviceStore.devices, 'condition')
    .filter(entry => entry.record.id !== props.modelValue.pumpSwitchDeviceId)
    .map(entry => ({ title: `${entry.config.name} #${entry.record.id}`, value: entry.record.id })),
])

const totalAmount = computed(() => totalScheduleAmount(props.modelValue.schedule.doses))

const { update } = useDraftModel<DosingPumpConfigDraft>(props, emit)

function updateContainer(patch: Partial<DosingPumpContainerDraft>): void {
  emit('update:modelValue', { ...props.modelValue, container: { ...props.modelValue.container, ...patch } })
}

function updateSchedule(patch: Partial<DosingPumpScheduleDraft>): void {
  emit('update:modelValue', { ...props.modelValue, schedule: { ...props.modelValue.schedule, ...patch } })
}

function toggleAllDoseDays(): void {
  updateSchedule({ daysOfWeek: props.modelValue.schedule.daysOfWeek.length === 7 ? [] : [0, 1, 2, 3, 4, 5, 6] })
}

// Changing the everyDays cycle re-anchors it to today (browser-local days-since-1970), so the
// firmware's cycle phase starts from "today counts as an active day".
function updateEveryDays(everyDays: number): void {
  updateSchedule({ everyDays, anchorDay: todayLocalDayNumber() })
}

function updateDose(index: number, patch: Partial<DosingPumpDoseDraft>): void {
  const doses = props.modelValue.schedule.doses.map((dose, doseIndex) => (doseIndex === index ? { ...dose, ...patch } : dose))
  updateSchedule({ doses })
}

function addDose(): void {
  if (props.modelValue.schedule.doses.length >= DOSING_PUMP_MAX_DOSES) {
    return
  }
  updateSchedule({ doses: [...props.modelValue.schedule.doses, { time: '12:00', amountMl: 5 }] })
}

function removeDose(index: number): void {
  updateSchedule({ doses: props.modelValue.schedule.doses.filter((_, doseIndex) => doseIndex !== index) })
}

// Generated rows replace the current dose list and remain fully editable before saving -
// correction-after-wizard is just editing the table (reference behavior).
function applyGeneratedDoses(doses: GeneratedDose[]): void {
  updateSchedule({ doses: doses.map(dose => ({ time: dose.time, amountMl: dose.amountMl })) })
}
</script>
