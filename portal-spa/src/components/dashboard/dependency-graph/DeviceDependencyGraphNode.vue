<template>
  <div class="position-relative">
    <Handle
      v-for="(pin, index) in hardwarePins"
      :id="`gpio-${pin.gpio}-${pin.label}`"
      :key="`target-${pin.gpio}-${pin.label}`"
      type="target"
      :position="targetPosition"
      :style="hardwareHandleStyle(index)"
      :connectable="false"
    />
    <Handle v-if="!hardwarePins.length" :id="`${id}-target`" type="target" :position="targetPosition" :connectable="false" />
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
import type { DeviceHardwarePin } from '@/models/devices/physical-connections'
import { resolveDeviceUi } from '@/components/devices/registry/device-ui-registry'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

export interface DeviceDependencyNodeData {
  deviceId: number
  direction?: 'LR' | 'TB' | 'RL' | 'BT'
  hardwarePins?: DeviceHardwarePin[]
}

const props = defineProps<NodeProps<DeviceDependencyNodeData>>()
const deviceStore = useDeviceRegistryStore()

defineEmits<{
  open: [deviceId: number]
  command: [deviceId: number, payload: DeviceCommandRequest]
}>()

const device = computed(() => deviceStore.devices.find(entry => entry.record.id === props.data.deviceId) ?? null)
const widgetComponent = computed(() => resolveDeviceUi(device.value?.record.typeName).widgetComponent)
const hardwarePins = computed(() => props.data.hardwarePins ?? [])
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

function hardwareHandleStyle(index: number): Record<string, string> {
  const count = hardwarePins.value.length
  const offset = count <= 1 ? 50 : 30 + (index * 40) / Math.max(1, count - 1)
  return props.data.direction === 'TB' || props.data.direction === 'BT'
    ? { left: `${offset}%` }
    : { top: `${offset}%` }
}
</script>
