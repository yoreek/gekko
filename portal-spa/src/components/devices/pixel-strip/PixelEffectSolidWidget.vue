<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-avatar :color="swatch" size="20" />
    </template>
    <div class="d-flex flex-column ga-2 w-100">
      <SwitchPowerButton :state="on" :disabled="editable || !isReady" size="small" @click.stop @toggle="setOn" />
      <PixelColorFields :model-value="liveColor" :disabled="editable || !isReady" @update:model-value="setColor" />
    </div>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column ga-3 pa-2">
    <SwitchPowerButton :state="on" :disabled="!isReady" size="default" @toggle="setOn" />
    <PixelColorFields :model-value="liveColor" :disabled="!isReady" @update:model-value="setColor" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { BaseDeviceRuntime, DeviceCommandRequest, DeviceRecord, PixelColor, PixelEffectSolidOutputSnapshot } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import SwitchPowerButton from '@/components/devices/common/SwitchPowerButton.vue'
import PixelColorFields from './PixelColorFields.vue'

interface PixelEffectSolidRuntime extends BaseDeviceRuntime {
  output?: PixelEffectSolidOutputSnapshot
}

const props = withDefaults(
  defineProps<{
    device: DeviceRecord<any, PixelEffectSolidRuntime>
    editable?: boolean
    dense?: boolean
  }>(),
  { dense: true },
)

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const liveColor = computed<PixelColor>(() => props.device.runtime.output?.color ?? { r: 0, g: 0, b: 0 })
const on = computed(() => props.device.runtime.output?.on ?? true)
const isReady = computed(() => props.device.runtime.effectiveStatus === 'ready')
const swatch = computed(() => `rgb(${liveColor.value.r}, ${liveColor.value.g}, ${liveColor.value.b})`)

function setColor(color: PixelColor): void {
  emit('command', { command: 'setOutput', state: color })
}

function setOn(value: boolean | null): void {
  emit('command', { command: 'setOutput', state: { on: Boolean(value) } })
}
</script>
