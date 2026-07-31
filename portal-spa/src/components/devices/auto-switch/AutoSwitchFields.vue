<template>
  <div class="d-flex flex-column ga-4">
    <v-alert v-if="switchItems.length === 0" type="warning" variant="tonal">
      {{ t('device.dialog.autoSwitch.noSwitch') }}
    </v-alert>

    <v-row>
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.targetSwitch')"
          :items="switchItems"
          :model-value="modelValue.targetSwitchDeviceId"
          :readonly="mode === 'view'"
          :disabled="(busy && mode !== 'view') || switchItems.length === 0"
          @update:model-value="update('targetSwitchDeviceId', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          min="1"
          :label="t('device.fields.pauseDurationSeconds')"
          :model-value="modelValue.pauseDurationSeconds"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('pauseDurationSeconds', Number($event))"
        />
      </v-col>
    </v-row>

    <div class="d-flex flex-column ga-3">
      <v-alert v-if="modelValue.conditions.length === 0" type="info" variant="tonal">
        {{ t('device.dialog.autoSwitch.noConditions') }}
      </v-alert>

      <v-card v-for="(condition, index) in modelValue.conditions" :key="index" variant="outlined" color="on-surface" class="pa-3">
        <v-row align="center">
          <v-col cols="12" sm="6">
            <v-select
              :label="t('device.fields.conditionDevice')"
              :items="conditionItemsFor(index)"
              :model-value="condition.deviceId"
              :readonly="mode === 'view'"
              :disabled="busy && mode !== 'view'"
              @update:model-value="updateCondition(index, { deviceId: Number($event) })"
            />
          </v-col>
          <v-col cols="6" sm="4">
            <v-switch
              :label="t('device.fields.conditionInvert')"
              :model-value="condition.invert"
              :readonly="mode === 'view'"
              :disabled="busy && mode !== 'view'"
              density="comfortable"
              hide-details
              inset
              @update:model-value="updateCondition(index, { invert: Boolean($event) })"
            />
          </v-col>
          <v-col cols="6" sm="2" class="d-flex justify-end">
            <v-btn
              v-if="mode !== 'view'"
              icon="trash"
              variant="text"
              density="comfortable"
              :disabled="busy"
              :aria-label="t('device.dialog.autoSwitch.removeCondition')"
              @click="removeCondition(index)"
            />
          </v-col>
        </v-row>
      </v-card>

      <v-btn
        v-if="mode !== 'view'"
        variant="tonal"
        prepend-icon="plus"
        :disabled="busy || modelValue.conditions.length >= AUTO_SWITCH_MAX_CONDITIONS"
        @click="addCondition"
      >
        {{ t('device.dialog.autoSwitch.addCondition') }}
      </v-btn>
      <div v-if="modelValue.conditions.length >= AUTO_SWITCH_MAX_CONDITIONS" class="text-body-small text-medium-emphasis">
        {{ t('device.dialog.autoSwitch.conditionsMaxReached') }}
      </div>
    </div>

    <div v-if="device" class="d-flex flex-column ga-3">
      <div class="d-flex flex-wrap ga-2">
        <v-chip variant="outlined">{{ t(switchStateLabelKey(state)) }}</v-chip>
      </div>

      <div class="text-label-small text-medium-emphasis">{{ t('device.dialog.quickCommands') }}</div>
      <div class="d-flex flex-wrap ga-2">
        <!-- Flat 4-way mode toggle (Off/On/Auto/Paused), mirrors ReefDuino's ScheduledSwitchMode -
             Paused is just another mode value, not a switch-style on/off nor a separate overlay
             with its own "resume" action. Exiting Paused is done via the same Off/On/Auto buttons
             that enter those modes from anywhere else. Paused itself is only reachable from Auto
             (disabled otherwise, mirroring handleCommand()'s guard). -->
        <v-btn-toggle :model-value="currentMode" mandatory="force" divided density="comfortable" :disabled="busy || !isReady">
          <v-btn value="off" @click="setMode('off')">{{ t(AutoSwitchDevice.modeLabelKey('off')) }}</v-btn>
          <v-btn value="on" @click="setMode('on')">{{ t(AutoSwitchDevice.modeLabelKey('on')) }}</v-btn>
          <v-btn value="auto" @click="setMode('auto')">{{ t(AutoSwitchDevice.modeLabelKey('auto')) }}</v-btn>
          <v-btn value="paused" :disabled="currentMode !== 'auto'" @click="setMode('paused')">{{ t(AutoSwitchDevice.modeLabelKey('paused')) }}</v-btn>
        </v-btn-toggle>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { AutoSwitchMode, AutoSwitchOutputSnapshot, DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import { switchStateLabelKey } from '@/models/devices/switch'
import { conditionDependencyOptions, dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import { AutoSwitchDevice, AUTO_SWITCH_MAX_CONDITIONS, type AutoSwitchConfigDraft, type AutoSwitchCondition } from '@/models/devices/auto-switch'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: AutoSwitchConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: AutoSwitchConfigDraft]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const switchItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'switch'))
// Excludes the currently-selected target switch and the device being edited itself, so an
// auto_switch can't be wired to condition on its own state (AutoSwitchDevice provides the
// Condition role, so its own id would otherwise be a selectable option).
const conditionItems = computed(() =>
  conditionDependencyOptions(deviceStore.devices, props.modelValue.targetSwitchDeviceId, props.device?.record.id),
)

// Excludes devices already picked by *other* condition rows, so the same device can't be added
// twice (mirrors the firmware/mock's "duplicate dependency device id" rejection). A row's own
// currently-selected device always stays in its own options list even if it would otherwise be
// excluded (e.g. by the target-switch exclusion above).
function conditionItemsFor(rowIndex: number): { title: string; value: number }[] {
  const selectedDeviceId = props.modelValue.conditions[rowIndex]?.deviceId ?? 0
  const usedByOtherRows = new Set(
    props.modelValue.conditions.filter((_, index) => index !== rowIndex).map(condition => condition.deviceId),
  )
  const options = conditionItems.value.filter(item => !usedByOtherRows.has(item.value))
  if (selectedDeviceId > 0 && !options.some(item => item.value === selectedDeviceId)) {
    const selected = deviceStore.devices.find(entry => entry.record.id === selectedDeviceId)
    if (selected) {
      return [{ title: `${selected.config.name} #${selected.record.id}`, value: selectedDeviceId }, ...options]
    }
  }
  return options
}

const output = computed(() => (props.device?.runtime as { output?: AutoSwitchOutputSnapshot } | undefined)?.output)
const isReady = computed(() => props.device?.runtime.effectiveStatus === 'ready')
const state = computed(() => output.value?.state ?? false)
const currentMode = computed<AutoSwitchMode>(() => output.value?.mode ?? 'auto')

const { update } = useDraftModel<AutoSwitchConfigDraft>(props, emit)

function updateCondition(index: number, patch: Partial<AutoSwitchCondition>): void {
  const conditions = props.modelValue.conditions.map((condition, conditionIndex) =>
    conditionIndex === index ? { ...condition, ...patch } : condition,
  )
  emit('update:modelValue', { ...props.modelValue, conditions })
}

function addCondition(): void {
  if (props.modelValue.conditions.length >= AUTO_SWITCH_MAX_CONDITIONS) {
    return
  }
  emit('update:modelValue', {
    ...props.modelValue,
    conditions: [...props.modelValue.conditions, { deviceId: 0, invert: false }],
  })
}

function removeCondition(index: number): void {
  emit('update:modelValue', {
    ...props.modelValue,
    conditions: props.modelValue.conditions.filter((_, conditionIndex) => conditionIndex !== index),
  })
}

function setMode(nextMode: AutoSwitchMode): void {
  if (nextMode === 'auto') {
    emit('command', { command: 'setMode', mode: 'auto' })
    return
  }
  if (nextMode === 'paused') {
    emit('command', { command: 'setMode', mode: 'pause' })
    return
  }
  emit('command', { command: 'setOutput', state: nextMode === 'on' })
}
</script>
