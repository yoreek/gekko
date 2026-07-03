<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="power" />
    </template>
    <SwitchPowerButton
      :state="state"
      :disabled="editable || !isReady"
      @toggle="onToggle"
    />
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column align-center ga-2 pa-2">
    <SwitchPowerButton
      :state="state"
      :disabled="!isReady"
      size="x-large"
      @toggle="onToggle"
    />
    <DeviceHistoryChart :device="device" :series-key="kSwitchStateSeries" class="w-100" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { BaseDeviceRuntime, DeviceCommandRequest, DeviceRecord, GpioSwitchOutputSnapshot } from '@/api/contracts'
import { switchCommandPayload, type OutputState } from '@/models/devices/switch'
import { kSwitchStateSeries } from '@/models/devices/history'
import DeviceWidgetBase from '@/v2/components/devices/common/DeviceWidgetBase.vue'
import SwitchPowerButton from '@/v2/components/devices/common/SwitchPowerButton.vue'
import DeviceHistoryChart from '@/v2/components/devices/common/DeviceHistoryChart.vue'

interface GpioSwitchRuntime extends BaseDeviceRuntime {
  output?: GpioSwitchOutputSnapshot
}

const props = withDefaults(defineProps<{
  device: DeviceRecord<any, GpioSwitchRuntime>
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

// `open`/`remove` are pure pass-through with no widget-specific logic — Vue's
// fallthrough attributes forward them to DeviceWidgetBase automatically since
// neither event is declared here. `command` is a real emit built from local
// state, so it stays explicit.
const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const state = computed<OutputState>(() => props.device.runtime.output?.state ?? 'disabled')
const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')

function onToggle(nextState: OutputState): void {
  emit('command', switchCommandPayload(nextState))
}
</script>
