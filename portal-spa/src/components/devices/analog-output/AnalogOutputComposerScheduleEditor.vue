<template>
  <AnalogScheduleChart
    v-if="compact && channelRecords.length > 0"
    compact
    :channels="chartChannels"
  />

  <section v-else-if="channelRecords.length > 0">
    <v-row>
      <v-col cols="12">
        <AnalogScheduleChart
          :channels="chartChannels"
          :editable="!readonly && scheduledChannels.length > 0"
          @update:channel-points="setChannelPoints"
        />
      </v-col>
    </v-row>

  </section>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'

import type { DeviceCommandRequest, DeviceRecord, ScheduledAnalogOutputOutputSnapshot } from '@/api/contracts'
import {
  flatAnalogSchedulePoints,
  type AnalogScheduleChartChannel,
} from '@/models/devices/analog-schedule'
import {
  ScheduledAnalogOutputDevice,
  type ScheduledAnalogOutputPointDraft,
} from '@/models/devices/composable-analog-output'
import AnalogScheduleChart from './AnalogScheduleChart.vue'

const props = withDefaults(defineProps<{
  channelRecords: DeviceRecord[]
  readonly?: boolean
  compact?: boolean
}>(), {
  readonly: false,
  compact: false,
})

const emit = defineEmits<{
  'update:pending-commands': [commands: DeviceCommandRequest[]]
}>()

const drafts = ref<Record<number, ScheduledAnalogOutputPointDraft[]>>({})

const scheduledChannels = computed(() =>
  props.channelRecords.filter(
    device => device.record.typeName === ScheduledAnalogOutputDevice.TYPE_NAME,
  ),
)

const channelRevisionKey = computed(() =>
  props.channelRecords
    .map(channel => `${channel.record.id}:${channel.record.configRevision}`)
    .join(','),
)

watch(
  channelRevisionKey,
  () => {
    const next: Record<number, ScheduledAnalogOutputPointDraft[]> = {}
    for (const channel of scheduledChannels.value) {
      next[channel.record.id] = new ScheduledAnalogOutputDevice()
        .normalizeConfig(channel.config, channel.config.deps)
        .points
    }
    drafts.value = next
  },
  { immediate: true },
)

const chartChannels = computed<AnalogScheduleChartChannel[]>(() =>
  props.channelRecords.map(channel => {
    const scheduled = channel.record.typeName === ScheduledAnalogOutputDevice.TYPE_NAME
    const state = ((channel.runtime as { output?: ScheduledAnalogOutputOutputSnapshot }).output?.state) ?? 0
    return {
      id: channel.record.id,
      name: channel.config.name,
      points: scheduled
        ? drafts.value[channel.record.id] ?? []
        : flatAnalogSchedulePoints(state),
      editable: scheduled,
    }
  }),
)

function setChannelPoints(deviceId: number, points: ScheduledAnalogOutputPointDraft[]): void {
  if (props.readonly || !(deviceId in drafts.value)) {
    return
  }
  drafts.value = { ...drafts.value, [deviceId]: points }
  emitPendingCommands()
}

function emitPendingCommands(): void {
  const commands: DeviceCommandRequest[] = []
  for (const channel of scheduledChannels.value) {
    const model = new ScheduledAnalogOutputDevice()
    for (const command of model.buildQuickUpdateCommands(channel, {
      points: drafts.value[channel.record.id] ?? [],
    })) {
      commands.push({ ...command, deviceId: channel.record.id })
    }
  }
  emit('update:pending-commands', commands)
}
</script>
