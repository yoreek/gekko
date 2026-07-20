<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="analog-input" />
    </template>
    <template v-if="reading?.valid">
      <div class="text-body-medium text-medium-emphasis">
        {{ voltageText }}
      </div>
    </template>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column align-center ga-1 pa-2">
    <div v-if="reading?.valid" class="text-headline-medium font-weight-bold text-high-emphasis">
      {{ voltageText }}
    </div>
    <div v-if="reading?.valid" class="text-body-small text-medium-emphasis">
      {{ new Date(reading.measuredAtMs ?? 0).toLocaleTimeString() }}
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { AnalogInputOutputSnapshot, BaseDeviceRuntime, DeviceRecord } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'

interface AnalogInputRuntime extends BaseDeviceRuntime {
  output?: AnalogInputOutputSnapshot
}

const props = withDefaults(defineProps<{
  device: DeviceRecord<any, AnalogInputRuntime>
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

const reading = computed(() => props.device.runtime.output?.analogInput)
const voltageText = computed(() => `${((reading.value?.milliVolts ?? 0) / 1000).toFixed(3)} V`)
</script>
