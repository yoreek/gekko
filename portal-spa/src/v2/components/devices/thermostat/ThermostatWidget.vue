<template>
  <v-card :title="device.config.name" :subtitle="t(modeLabel)" density="compact">
    <template #prepend>
      <v-icon icon="temperature" />
    </template>
    <template #append>
      <v-chip :color="statusColor" size="small" variant="tonal">
        {{ controlStatus }}
      </v-chip>
    </template>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { BaseDeviceRuntime, DeviceRecord, ThermostatOutputSnapshot } from '@/api/contracts'

interface ThermostatRuntime extends BaseDeviceRuntime {
  output?: ThermostatOutputSnapshot
}

const props = defineProps<{
  device: DeviceRecord<any, ThermostatRuntime>
}>()

const { t } = useI18n()

const modeLabel = computed(() => {
  const mode = (props.device.config as Record<string, unknown>).mode ?? 'off'
  return `device.fields.mode`
})

const controlStatus = computed(() => props.device.runtime.output?.controlStatus ?? 'idle')

const statusColor = computed(() => {
  const desiredState = props.device.runtime.output?.desiredSwitchState
  return desiredState === 'on' ? 'warning' : 'secondary'
})
</script>
