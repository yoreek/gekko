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
      <v-col v-if="device" cols="12">
        <v-btn-toggle
          :model-value="output.mode ?? 'scheduled'"
          mandatory="force"
          divided
          :disabled="busy || device.runtime.effectiveStatus !== 'ready'"
          @update:model-value="$emit('command', { command: 'setMode', mode: String($event) })"
        >
          <v-btn value="off">{{ t('device.mode.off') }}</v-btn>
          <v-btn value="scheduled">{{ t('device.mode.scheduled') }}</v-btn>
          <v-btn value="manual">{{ t('device.mode.manual') }}</v-btn>
        </v-btn-toggle>
      </v-col>
    </v-row>
    <v-row>
      <v-col cols="12">
        <AnalogScheduleChart
          :channels="chartChannels"
          :editable="mode !== 'view'"
          @update:channel-points="updateChartPoints"
        />
      </v-col>
    </v-row>
    <v-row v-if="device && mode !== 'view'">
      <v-col cols="12">
        <SchedulePresetSelect
          :device-id="device.record.id"
          :points="modelValue.points"
          :disabled="busy"
          @apply="applyPreset"
        />
      </v-col>
    </v-row>
    <v-row v-if="device">
      <v-col cols="12">
        <AnalogOutputLevelControl
          :model-value="output.state ?? 0"
          :label="t('device.fields.currentOutput')"
          :disabled="busy || device.runtime.effectiveStatus !== 'ready'"
          :debounce-ms="300"
          @update:model-value="$emit('command', { command: 'setOutput', state: $event })"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.requestedOutput')" :model-value="`${output.requestedState ?? 0}%`" readonly />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.timeValid')" :model-value="output.timeValid ? t('labels.yes') : t('labels.no')" readonly />
      </v-col>
    </v-row>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { BaseDeviceRuntime, DeviceCommandRequest, DeviceRecord, ScheduledAnalogOutputOutputSnapshot } from '@/api/contracts'
import type { AnalogScheduleChartChannel } from '@/models/devices/analog-schedule'
import type { ScheduledAnalogOutputConfigDraft, ScheduledAnalogOutputPointDraft } from '@/models/devices/composable-analog-output'
import AnalogScheduleChart from './AnalogScheduleChart.vue'
import AnalogOutputLevelControl from './AnalogOutputLevelControl.vue'
import AnalogOutputTargetSelect from './AnalogOutputTargetSelect.vue'
import SchedulePresetSelect from './SchedulePresetSelect.vue'
import { useDraftModel } from '@/composables/useDraftModel'

interface ScheduledAnalogOutputRuntime extends BaseDeviceRuntime {
  output?: ScheduledAnalogOutputOutputSnapshot
}
const props = defineProps<{ modelValue: ScheduledAnalogOutputConfigDraft; device?: DeviceRecord<any, ScheduledAnalogOutputRuntime>; mode: 'view' | 'edit' | 'create'; busy?: boolean }>()
const emit = defineEmits<{ 'update:modelValue': [value: ScheduledAnalogOutputConfigDraft]; command: [payload: DeviceCommandRequest] }>()
const { t } = useI18n()
const { update } = useDraftModel<ScheduledAnalogOutputConfigDraft>(props, emit)
const output = computed(() => props.device?.runtime.output ?? {})
const chartChannels = computed<AnalogScheduleChartChannel[]>(() => [{
  id: props.device?.record.id ?? 0,
  name: props.modelValue.name,
  points: props.modelValue.points,
  editable: true,
}])
function updateChartPoints(_channelId: number, points: ScheduledAnalogOutputPointDraft[]): void {
  update('points', points)
}
function applyPreset(points: ScheduledAnalogOutputPointDraft[]): void {
  update('points', points)
}
</script>
