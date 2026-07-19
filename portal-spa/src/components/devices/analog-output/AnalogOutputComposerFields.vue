<template>
  <section>
    <v-row>
      <v-col cols="12">
        <v-select
          multiple
          chips
          :label="t('device.fields.composerChannels')"
          :items="dependencyItems"
          :model-value="modelValue.targetDeviceIds"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('targetDeviceIds', ($event as number[]).map(Number))"
        />
      </v-col>
      <v-col v-if="device" cols="12">
        <v-btn-toggle
          :model-value="output.mode"
          divided
          mandatory="force"
          :disabled="busy || device.runtime.effectiveStatus !== 'ready'"
          @update:model-value="$emit('command', { command: 'setMode', mode: String($event) })"
        >
          <v-btn value="off">{{ t('device.mode.off') }}</v-btn>
          <v-btn value="scheduled">{{ t('device.mode.scheduled') }}</v-btn>
          <v-btn value="manual">{{ t('device.mode.manual') }}</v-btn>
        </v-btn-toggle>
      </v-col>
    </v-row>

    <v-row v-if="device && output.mode === 'manual' && controlChannels.length > 0">
      <v-col cols="12">
        <AnalogOutputComposerChannelControls
          :channels="controlChannels"
          :disabled="busy || device.runtime.effectiveStatus !== 'ready'"
          @update="(channel, state) => $emit('command', { deviceId: controlChannels[channel].id, command: 'setOutput', state })"
        />
      </v-col>
    </v-row>

    <v-row v-if="device && output.mode === 'scheduled' && controlChannels.length > 0">
      <v-col cols="12">
        <AnalogOutputComposerChannelStatus :channels="controlChannels" />
      </v-col>
    </v-row>

    <v-row v-if="channelRecords.length > 0" class="mt-4">
      <v-col cols="12">
        <AnalogOutputComposerScheduleEditor
          :channel-records="channelRecords"
          :readonly="mode === 'view'"
          @update:pending-commands="update('pendingCommands', $event)"
        />
      </v-col>
    </v-row>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type {
  AnalogOutputComposerOutputSnapshot,
  BaseDeviceRuntime,
  DeviceCommandRequest,
  DeviceDependencyLink,
  DeviceRecord,
  ScheduledAnalogOutputOutputSnapshot,
} from '@/api/contracts'
import type {
  AnalogOutputComposerConfigDraft,
} from '@/models/devices/composable-analog-output'
import { exclusiveAnalogOutputDependencyOptions } from '@/models/devices/device-model-factory'
import { useDraftModel } from '@/composables/useDraftModel'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import AnalogOutputComposerChannelControls from './AnalogOutputComposerChannelControls.vue'
import AnalogOutputComposerChannelStatus from './AnalogOutputComposerChannelStatus.vue'
import AnalogOutputComposerScheduleEditor from './AnalogOutputComposerScheduleEditor.vue'

interface AnalogOutputComposerRuntime extends BaseDeviceRuntime {
  output?: AnalogOutputComposerOutputSnapshot
}

const props = defineProps<{
  modelValue: AnalogOutputComposerConfigDraft
  device?: DeviceRecord<any, AnalogOutputComposerRuntime>
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: AnalogOutputComposerConfigDraft]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const store = useDeviceRegistryStore()
const { update } = useDraftModel<AnalogOutputComposerConfigDraft>(props, emit)
const output = computed(() => props.device?.runtime.output ?? {})
const dependencyItems = computed(() =>
  exclusiveAnalogOutputDependencyOptions(store.devices, props.device?.record.id),
)
const channelRecords = computed<DeviceRecord[]>(() =>
  ((props.device?.config.deps ?? []) as DeviceDependencyLink[])
    .filter(link => link.role === 'analog_output')
    .flatMap(link => {
      const record = store.devices.find(device => device.record.id === link.deviceId)
      return record === undefined ? [] : [record]
    }),
)
const controlChannels = computed(() =>
  channelRecords.value.map((record, index) => {
    const channelOutput = (record.runtime as BaseDeviceRuntime & { output?: ScheduledAnalogOutputOutputSnapshot }).output ?? {}
    return {
      id: record.record.id,
      index,
      name: record.config.name,
      state: channelOutput.state ?? 0,
      requestedState: channelOutput.requestedState ?? channelOutput.state ?? 0,
    }
  }),
)
</script>
