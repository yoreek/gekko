<template>
  <v-card :title="device.config.name" density="compact">
    <template #prepend>
      <v-icon icon="power" />
    </template>
    <template #append>
      <v-chip :color="stateColor" size="small" variant="tonal">
        {{ t(outputStateLabelKey(state)) }}
      </v-chip>
    </template>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { BaseDeviceRuntime, DeviceRecord, GpioSwitchOutputSnapshot } from '@/api/contracts'
import { outputStateLabelKey, type OutputState } from '@/models/devices/switch'

interface GpioSwitchRuntime extends BaseDeviceRuntime {
  output?: GpioSwitchOutputSnapshot
}

const props = defineProps<{
  device: DeviceRecord<any, GpioSwitchRuntime>
}>()

const { t } = useI18n()

const state = computed<OutputState>(() => props.device.runtime.output?.state ?? 'disabled')

const stateColor = computed(() => {
  switch (state.value) {
    case 'on':
      return 'success'
    case 'off':
      return 'secondary'
    default:
      return 'warning'
  }
})
</script>
