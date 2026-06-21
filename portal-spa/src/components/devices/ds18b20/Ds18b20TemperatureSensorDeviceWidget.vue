<template>
  <DeviceWidgetBase :device="device" :editable="editable" @open="$emit('open')">
    <template #actions>
      <v-chip size="small" variant="tonal" :color="temperature?.valid ? 'primary' : 'secondary'">
        {{ temperatureText }}
      </v-chip>
    </template>
  </DeviceWidgetBase>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import DeviceWidgetBase from '@/components/devices/base/DeviceWidgetBase.vue'
import type { DashboardDevice } from '@/models/device'
import { formatTemperatureOutput, temperatureOutputValid } from '@/models/devices/ds18b20'

const props = defineProps<{
  device: DashboardDevice
  editable?: boolean
}>()

defineEmits<{
  open: []
}>()

const { t } = useI18n()
const temperature = computed(() => {
  const value = props.device.output.temperature
  return temperatureOutputValid(value) ? value : undefined
})
const temperatureText = computed(() => formatTemperatureOutput(temperature.value) || t('device.dialog.temperatureUnavailableShort'))
</script>
