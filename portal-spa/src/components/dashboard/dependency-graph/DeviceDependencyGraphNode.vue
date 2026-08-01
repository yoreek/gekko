<template>
  <div class="position-relative">
    <Handle :id="`${id}-target`" type="target" :position="targetPosition" :connectable="false" />
    <component
      v-if="device"
      :is="widgetComponent"
      :device="device"
      :editable="false"
      :dense="true"
      @open="$emit('open', device.record.id)"
      @command="$emit('command', device.record.id, $event)"
    />
    <Handle :id="`${id}-source`" type="source" :position="sourcePosition" :connectable="false" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { Handle, Position, type NodeProps } from '@vue-flow/core'

import type { DeviceCommandRequest } from '@/api/contracts'
import { resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

export interface DeviceDependencyNodeData {
  deviceId: number
  direction?: 'LR' | 'TB' | 'RL' | 'BT'
}

const props = defineProps<NodeProps<DeviceDependencyNodeData>>()
const deviceStore = useDeviceRegistryStore()

defineEmits<{
  open: [deviceId: number]
  command: [deviceId: number, payload: DeviceCommandRequest]
}>()

const device = computed(() => deviceStore.devices.find(entry => entry.record.id === props.data.deviceId) ?? null)
const widgetComponent = computed(() => resolveDeviceUi(device.value?.record.typeName).widgetComponent)
const targetPosition = computed(() => {
  if (props.data.direction === 'TB') return Position.Top
  if (props.data.direction === 'BT') return Position.Bottom
  if (props.data.direction === 'RL') return Position.Right
  return Position.Left
})
const sourcePosition = computed(() => {
  if (props.data.direction === 'TB') return Position.Bottom
  if (props.data.direction === 'BT') return Position.Top
  if (props.data.direction === 'RL') return Position.Left
  return Position.Right
})
</script>
