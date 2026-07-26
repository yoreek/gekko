<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="temperature" />
    </template>
    <template v-if="temp || humidity">
      <div class="text-body-medium text-medium-emphasis">
        <span v-if="temp">{{ temp.value.toFixed(1) }}{{ temp.unitSymbol }}</span>
        <span v-if="temp && humidity"> · </span>
        <span v-if="humidity">{{ humidity.value.toFixed(1) }}{{ humidity.unitSymbol }}</span>
      </div>
    </template>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column align-center ga-1 pa-2">
    <div v-if="temp" class="text-headline-medium font-weight-bold text-high-emphasis">
      {{ temp.value.toFixed(1) }}{{ temp.unitSymbol }}
    </div>
    <div v-if="humidity" class="text-title-medium text-high-emphasis">
      {{ humidity.value.toFixed(1) }}{{ humidity.unitSymbol }}
    </div>
    <div v-if="temp" class="text-body-small text-medium-emphasis">
      {{ new Date(temp.measuredAtMs).toLocaleTimeString() }}
    </div>
    <DeviceHistoryChart :device="device" :series-key="kTemperatureSeries" class="w-100" />
    <DeviceHistoryChart :device="device" :series-key="kHumiditySeries" class="w-100" />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type {
  BaseDeviceRuntime,
  DeviceRecord,
  Aht10SensorOutputSnapshot,
  HumidityOutputSnapshot,
  TemperatureOutputSnapshot,
} from '@/api/contracts'
import { kHumiditySeries, kTemperatureSeries } from '@/models/devices/history'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'
import DeviceHistoryChart from '@/components/devices/common/DeviceHistoryChart.vue'

interface Aht10Runtime extends BaseDeviceRuntime {
  output?: Aht10SensorOutputSnapshot
}

const props = withDefaults(defineProps<{
  device: DeviceRecord<any, Aht10Runtime>
  editable?: boolean
  dense?: boolean
}>(), {
  dense: true,
})

// `open`/`remove` are pure pass-through with no widget-specific logic — Vue's
// fallthrough attributes forward them to DeviceWidgetBase automatically since
// neither event is declared here.
const temp = computed<TemperatureOutputSnapshot | undefined>(() => props.device.runtime.output?.temperature)
const humidity = computed<HumidityOutputSnapshot | undefined>(() => props.device.runtime.output?.humidity)
</script>
