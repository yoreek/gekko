<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="analog-output" />
    </template>
    <AnalogOutputComposerScheduleEditor
      compact
      readonly
      :channel-records="channelRecords"
    />
  </DeviceWidgetBase>

  <section v-else>
    <v-row>
      <v-col cols="12">
        <v-btn-toggle
          :model-value="output.mode ?? 'scheduled'"
          divided
          mandatory="force"
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
        <AnalogOutputComposerScheduleEditor
          :channel-records="channelRecords"
          :readonly="editable || !isReady"
          @update:pending-commands="pendingCommands = $event"
        />
      </v-col>
    </v-row>

    <v-row v-if="pendingCommands.length > 0">
      <v-col cols="12">
        <v-btn color="primary" :disabled="editable || !isReady" @click="saveSchedule">
          {{ t('actions.save') }}
        </v-btn>
      </v-col>
    </v-row>

    <v-row v-if="output.mode === 'scheduled' && controlChannels.length > 0" class="mt-4">
      <v-col cols="12">
        <AnalogOutputComposerChannelStatus :channels="controlChannels" />
      </v-col>
    </v-row>

    <v-row v-if="output.mode === 'manual' && controlChannels.length > 0" class="mt-4">
      <v-col cols="12">
        <AnalogOutputComposerChannelControls
          :channels="controlChannels"
          :disabled="editable || !isReady"
          @update="(channel, state) => $emit('command', { deviceId: controlChannels[channel].id, command: 'setOutput', state })"
        />
      </v-col>
    </v-row>
  </section>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'

import type {
  AnalogOutputComposerOutputSnapshot,
  BaseDeviceRuntime,
  DeviceCommandRequest,
  DeviceDependencyLink,
  DeviceRecord,
  ScheduledAnalogOutputOutputSnapshot,
} from '@/api/contracts'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import AnalogOutputComposerChannelControls from './AnalogOutputComposerChannelControls.vue'
import AnalogOutputComposerChannelStatus from './AnalogOutputComposerChannelStatus.vue'
import AnalogOutputComposerScheduleEditor from './AnalogOutputComposerScheduleEditor.vue'

interface AnalogOutputComposerRuntime extends BaseDeviceRuntime {
  output?: AnalogOutputComposerOutputSnapshot
}

const props = withDefaults(defineProps<{
  device: DeviceRecord<any, AnalogOutputComposerRuntime>
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
const store = useDeviceRegistryStore()
const pendingCommands = ref<DeviceCommandRequest[]>([])

function saveSchedule(): void {
  for (const command of pendingCommands.value) {
    emit('command', command)
  }
  pendingCommands.value = []
}
const output = computed(() =>
  (props.device.runtime.output ?? {}) as AnalogOutputComposerOutputSnapshot,
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
const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')
const channelRecords = computed<DeviceRecord[]>(() =>
  (props.device.config.deps as DeviceDependencyLink[])
    .filter(link => link.role === 'analog_output')
    .flatMap(link => {
      const record = store.devices.find(device => device.record.id === link.deviceId)
      return record === undefined ? [] : [record]
    }),
)
</script>
