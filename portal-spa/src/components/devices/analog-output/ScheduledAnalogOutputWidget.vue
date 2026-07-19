<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="analog-output" />
    </template>
    <AnalogScheduleChart compact :channels="chartChannels" />
  </DeviceWidgetBase>

  <section v-else>
    <v-row>
      <v-col cols="12">
        <v-btn-toggle
          :model-value="output.mode ?? 'scheduled'"
          mandatory="force"
          divided
          :disabled="editable || !isReady"
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
          :editable="!editable && isReady"
          @update:channel-points="setSchedulePoints"
        />
      </v-col>
    </v-row>
    <v-row v-if="scheduleDirty">
      <v-col cols="12">
        <v-btn
          color="primary"
          :disabled="editable || !isReady || !scheduleDirty"
          @click="saveSchedule"
        >
          {{ t('actions.save') }}
        </v-btn>
      </v-col>
    </v-row>
    <v-row v-if="output.mode === 'scheduled'" class="mt-4">
      <v-col cols="12">
        <AnalogOutputComposerChannelStatus :channels="controlChannels" />
      </v-col>
    </v-row>
    <v-row v-if="output.mode === 'manual'" class="mt-4">
      <v-col cols="12">
        <AnalogOutputComposerChannelControls
          :channels="controlChannels"
          :disabled="editable || !isReady"
          @update="(_channel, state) => $emit('command', { command: 'setOutput', state })"
        />
      </v-col>
    </v-row>
  </section>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

import type {
  BaseDeviceRuntime,
  DeviceCommandRequest,
  DeviceRecord,
  ScheduledAnalogOutputOutputSnapshot,
} from '@/api/contracts'
import type { AnalogScheduleChartChannel } from '@/models/devices/analog-schedule'
import {
  ScheduledAnalogOutputDevice,
  type ScheduledAnalogOutputPointDraft,
} from '@/models/devices/composable-analog-output'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import AnalogOutputComposerChannelControls from './AnalogOutputComposerChannelControls.vue'
import AnalogOutputComposerChannelStatus from './AnalogOutputComposerChannelStatus.vue'
import AnalogScheduleChart from './AnalogScheduleChart.vue'

interface ScheduledAnalogOutputRuntime extends BaseDeviceRuntime {
  output?: ScheduledAnalogOutputOutputSnapshot
}

const props = withDefaults(defineProps<{
  device: DeviceRecord<any, ScheduledAnalogOutputRuntime>
  editable?: boolean
  dense?: boolean
}>(), {
  editable: false,
  dense: true,
})

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const model = new ScheduledAnalogOutputDevice()
const schedulePoints = ref<ScheduledAnalogOutputPointDraft[]>([])
const output = computed(() => props.device.runtime.output ?? {})
const controlChannels = computed(() => [{
  id: props.device.record.id,
  index: 0,
  name: props.device.config.name,
  state: output.value.state ?? 0,
  requestedState: output.value.requestedState ?? output.value.state ?? 0,
}])
const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')
const configRevision = computed(() => props.device.record.configRevision)
const persistedSchedulePoints = computed(() =>
  model.normalizeConfig(props.device.config, props.device.config.deps).points,
)
const scheduleDirty = computed(() =>
  JSON.stringify(schedulePoints.value) !== JSON.stringify(persistedSchedulePoints.value),
)
const chartChannels = computed<AnalogScheduleChartChannel[]>(() => [{
  id: props.device.record.id,
  name: props.device.config.name,
  points: schedulePoints.value,
  editable: true,
}])

watch(
  configRevision,
  () => {
    schedulePoints.value = persistedSchedulePoints.value
  },
  { immediate: true },
)

function setSchedulePoints(
  _channelId: number,
  points: ScheduledAnalogOutputPointDraft[],
): void {
  schedulePoints.value = points
}

function saveSchedule(): void {
  for (const command of model.buildQuickUpdateCommands(props.device, {
    points: schedulePoints.value,
  })) {
    emit('command', command)
  }
}
</script>
