<template>
  <section class="d-flex flex-column ga-4">
    <v-row v-if="device">
      <v-col cols="12">
        <v-text-field
          :label="t('device.fields.currentOutput')"
          :model-value="`${Math.round(liveStatePercent)}%`"
          readonly
        />
      </v-col>
    </v-row>

    <v-row>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.analogPin')"
          :model-value="modelValue.pin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('pin', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.analogLedcChannel')"
          :model-value="modelValue.ledcChannel"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('ledcChannel', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.analogFrequencyHz')"
          :model-value="modelValue.frequencyHz"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          min="1"
          @update:model-value="update('frequencyHz', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.analogDutyBits')"
          :model-value="modelValue.dutyBits"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          min="1"
          max="20"
          @update:model-value="update('dutyBits', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.analogStartupStatePercent')"
          :model-value="modelValue.startupState"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          min="0"
          max="100"
          @update:model-value="update('startupState', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.analogSafeStatePercent')"
          :model-value="modelValue.safeState"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          min="0"
          max="100"
          @update:model-value="update('safeState', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-switch
          :label="t('device.fields.analogInverted')"
          :model-value="modelValue.inverted"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          inset
          @update:model-value="update('inverted', Boolean($event))"
        />
      </v-col>
      <v-col cols="12">
        <v-switch
          :label="t('device.fields.restorePreviousState')"
          :model-value="modelValue.restorePreviousState"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          inset
          @update:model-value="update('restorePreviousState', Boolean($event))"
        />
      </v-col>
    </v-row>

    <v-row v-if="device">
      <v-col cols="12">
        <AnalogOutputLevelControl
          :model-value="liveStatePercent"
          :label="t('device.fields.currentOutput')"
          :disabled="busy || !isReady"
          @update:model-value="setOutputState"
        />
      </v-col>
    </v-row>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { AnalogOutputOutputSnapshot, DeviceCommandRequest, DeviceRecord } from '@/api/contracts'
import AnalogOutputLevelControl from '@/components/devices/analog-output/AnalogOutputLevelControl.vue'
import type { AnalogOutputConfigDraft } from '@/models/devices/analog-output'

const props = defineProps<{
  modelValue: AnalogOutputConfigDraft
  mode: 'create' | 'edit' | 'view'
  busy: boolean
  device?: DeviceRecord
}>()

const emit = defineEmits<{
  'update:modelValue': [value: AnalogOutputConfigDraft]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()

const liveStatePercent = computed(
  () => (props.device?.runtime as { output?: AnalogOutputOutputSnapshot } | undefined)?.output?.state ?? 0,
)
const isReady = computed(() => props.device?.runtime.effectiveStatus === 'ready')

function update<K extends keyof AnalogOutputConfigDraft>(key: K, value: AnalogOutputConfigDraft[K]): void {
  emit('update:modelValue', {
    ...props.modelValue,
    [key]: value,
  })
}

function setOutputState(state: number): void {
  emit('command', {
    command: 'setOutput',
    state,
  })
}
</script>
