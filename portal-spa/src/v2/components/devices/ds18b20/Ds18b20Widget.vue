<template>
  <v-card :title="device.config.name" density="compact">
    <template #prepend>
      <v-icon icon="temperature" />
    </template>
    <template #append>
      <div v-if="temp" class="text-body-2 text-medium-emphasis">
        {{ temp.value.toFixed(1) }}{{ temp.unitSymbol }}
      </div>
    </template>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { BaseDeviceRuntime, DeviceRecord, Ds18b20TemperatureSensorOutputSnapshot, TemperatureOutputSnapshot } from '@/api/contracts'

interface Ds18b20Runtime extends BaseDeviceRuntime {
  output?: Ds18b20TemperatureSensorOutputSnapshot
}

const props = defineProps<{
  device: DeviceRecord<any, Ds18b20Runtime>
}>()

const temp = computed<TemperatureOutputSnapshot | undefined>(() => props.device.runtime.output?.temperature)
</script>
