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
    <v-chip v-if="oscillatorStopped" color="warning" variant="tonal" size="small" class="mt-2">
      {{ t('device.dialog.rtcDs3231.oscillatorStoppedWarning') }}
    </v-chip>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { BaseDeviceRuntime, DeviceRecord, RtcDs3231OutputSnapshot } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/common/DeviceWidgetBase.vue'

interface RtcDs3231Runtime extends BaseDeviceRuntime, RtcDs3231OutputSnapshot {}

const props = withDefaults(
  defineProps<{
    device: DeviceRecord<any, RtcDs3231Runtime>
    editable?: boolean
    dense?: boolean
  }>(),
  {
    dense: true,
  },
)

const { t } = useI18n()

const currentTimeText = computed(() => {
  const epoch = props.device.runtime.currentEpochUtc
  return epoch ? new Date(epoch * 1000).toLocaleString() : ''
})
const oscillatorStopped = computed(() => props.device.runtime.oscillatorStopped === true)
</script>
