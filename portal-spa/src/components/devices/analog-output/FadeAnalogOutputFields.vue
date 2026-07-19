<template>
  <section>
    <v-row>
      <v-col cols="12">
        <AnalogOutputTargetSelect
          :model-value="modelValue.targetDeviceId"
          :owner-device-id="device?.record.id"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('targetDeviceId', $event)"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          min="1"
          max="100"
          suffix="%"
          :label="t('device.fields.fadeMaxStep')"
          :model-value="modelValue.maxStep"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('maxStep', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          min="1"
          max="60000"
          :label="t('device.fields.fadeStepIntervalMs')"
          :model-value="modelValue.stepIntervalMs"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('stepIntervalMs', Number($event))"
        />
      </v-col>
    </v-row>
    <v-row v-if="device">
      <v-col cols="12">
        <AnalogOutputLevelControl
          :model-value="output.state ?? 0"
          :label="t('device.fields.targetOutput')"
          :disabled="busy || device.runtime.effectiveStatus !== 'ready'"
          :debounce-ms="300"
          @update:model-value="$emit('command', { command: 'setOutput', state: $event })"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.currentOutput')" :model-value="`${output.state ?? 0}%`" readonly />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.targetOutput')" :model-value="`${output.targetState ?? 0}%`" readonly />
      </v-col>
    </v-row>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { BaseDeviceRuntime, DeviceCommandRequest, DeviceRecord, FadeAnalogOutputOutputSnapshot } from '@/api/contracts'
import type { FadeAnalogOutputConfigDraft } from '@/models/devices/composable-analog-output'
import AnalogOutputLevelControl from './AnalogOutputLevelControl.vue'
import AnalogOutputTargetSelect from './AnalogOutputTargetSelect.vue'
import { useDraftModel } from '@/composables/useDraftModel'

interface FadeAnalogOutputRuntime extends BaseDeviceRuntime {
  output?: FadeAnalogOutputOutputSnapshot
}
const props = defineProps<{ modelValue: FadeAnalogOutputConfigDraft; device?: DeviceRecord<any, FadeAnalogOutputRuntime>; mode: 'view' | 'edit' | 'create'; busy?: boolean }>()
const emit = defineEmits<{ 'update:modelValue': [value: FadeAnalogOutputConfigDraft]; command: [payload: DeviceCommandRequest] }>()
const { t } = useI18n()
const { update } = useDraftModel<FadeAnalogOutputConfigDraft>(props, emit)
const output = computed(() => props.device?.runtime.output ?? {})
</script>
