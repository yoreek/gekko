<template>
  <SwitchDeviceWidgetBase :device="device" :editable="editable" @open="$emit('open')">
    <template #actions>
      <SwitchPowerButton
        :state="outputState"
        :disabled="editable || !device.isReady"
        @toggle="state => $emit('command', switchCommandPayload(state))"
      />
    </template>
  </SwitchDeviceWidgetBase>
</template>

<script setup lang="ts">
import { computed } from 'vue'

import type { DeviceCommandRequest } from '@/api'
import SwitchDeviceWidgetBase from '@/components/devices/switch/SwitchDeviceWidgetBase.vue'
import SwitchPowerButton from '@/components/devices/switch/SwitchPowerButton.vue'
import type { DashboardDevice } from '@/models/device'
import { isOutputState, switchCommandPayload } from '@/models/devices/switch'

const props = defineProps<{
  device: DashboardDevice
  editable?: boolean
}>()

defineEmits<{
  open: []
  command: [payload: DeviceCommandRequest]
}>()

const outputState = computed(() => (isOutputState(props.device.output.state) ? props.device.output.state : undefined))
</script>
