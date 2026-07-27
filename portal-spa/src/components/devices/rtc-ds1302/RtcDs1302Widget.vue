<template>
  <DeviceWidgetBase v-if="dense" :device="device" :editable="editable">
    <template #prepend>
      <v-icon icon="time" />
    </template>
    <template v-if="currentTimeText">
      <div class="text-body-medium text-medium-emphasis">{{ currentTimeText }}</div>
    </template>
  </DeviceWidgetBase>

  <div v-else class="d-flex flex-column align-center ga-1 pa-2">
    <div class="text-headline-medium font-weight-bold text-high-emphasis">{{ currentTimeText || '—' }}</div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { BaseDeviceRuntime, DeviceRecord, RtcDs1302OutputSnapshot } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'

interface RtcDs1302Runtime extends BaseDeviceRuntime, RtcDs1302OutputSnapshot {}

const props = withDefaults(
  defineProps<{
    device: DeviceRecord<any, RtcDs1302Runtime>
    editable?: boolean
    dense?: boolean
  }>(),
  {
    dense: true,
  },
)

const currentTimeText = computed(() => {
  const epoch = props.device.runtime.currentEpochUtc
  return epoch ? new Date(epoch * 1000).toLocaleString() : ''
})
</script>
