<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="temperature" />
    </template>
    <template v-if="temp">
      <div class="text-body-medium text-medium-emphasis">
        {{ temp.value.toFixed(1) }}{{ temp.unitSymbol }}
      </div>
    </template>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column align-center ga-1 pa-2">
    <div v-if="temp" class="text-headline-medium font-weight-bold text-high-emphasis">
      {{ temp.value.toFixed(1) }}{{ temp.unitSymbol }}
    </div>
    <div v-if="temp" class="text-body-small text-medium-emphasis">
      {{ new Date(temp.measuredAtMs).toLocaleTimeString() }}
    </div>
    <DeviceHistoryChart :device="device" :series-key="kTemperatureSeries" class="w-100" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { BaseDeviceRuntime, DeviceRecord, NtcThermistorTemperatureSensorOutputSnapshot, TemperatureOutputSnapshot } from '@/api/contracts'
import { kTemperatureSeries } from '@/models/devices/history'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import DeviceHistoryChart from '@/components/devices/common/DeviceHistoryChart.vue'

interface NtcThermistorRuntime extends BaseDeviceRuntime {
  output?: NtcThermistorTemperatureSensorOutputSnapshot
}

const props = withDefaults(defineProps<{
  device: DeviceRecord<any, NtcThermistorRuntime>
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

// `open`/`remove` are pure pass-through with no widget-specific logic — Vue's
// fallthrough attributes forward them to DeviceWidgetBase automatically since
// neither event is declared here.
const temp = computed<TemperatureOutputSnapshot | undefined>(() => props.device.runtime.output?.temperature)
</script>
