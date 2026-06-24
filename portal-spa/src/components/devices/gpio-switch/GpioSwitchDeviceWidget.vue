<template>
  <SwitchDeviceWidgetBase :device="device" :editable="editable" @open="$emit('open')">
    <template #actions>
    <SwitchPowerButton
      :state="outputState"
      :disabled="editable || !isReady"
      @toggle="state => $emit('command', switchCommandPayload(state))"
    />
    </template>
  </SwitchDeviceWidgetBase>
</template>

<script setup lang="ts">
import { computed } from 'vue'

import type { DeviceCommandRequest } from '@/api'
import type { GpioSwitchOutputSnapshot, DeviceRecord } from '@/api/contracts'
import SwitchDeviceWidgetBase from '@/components/devices/switch/SwitchDeviceWidgetBase.vue'
import SwitchPowerButton from '@/components/devices/switch/SwitchPowerButton.vue'
import { deviceRecordEffectiveStatus } from '@/models/device'
import { isOutputState, switchCommandPayload } from '@/models/devices/switch'

const props = defineProps<{
  device: DeviceRecord
  editable?: boolean
}>()

defineEmits<{
  open: []
  command: [payload: DeviceCommandRequest]
}>()

const outputState = computed(() => {
  const output = props.device.runtime as GpioSwitchOutputSnapshot
  const state = output.state
  return isOutputState(state) ? state : undefined
})

const isReady = computed(() => deviceRecordEffectiveStatus(props.device) === 'ready')
</script>
