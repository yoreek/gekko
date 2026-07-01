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

import type { DeviceRecord, Ds18b20TemperatureSensorOutputSnapshot, TemperatureOutputSnapshot } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/base/DeviceWidgetBase.vue'
import { Ds18b20Device } from '@/models/devices/ds18b20'

const props = defineProps<{
  device: DeviceRecord
  editable?: boolean
}>()

defineEmits<{
  open: []
}>()

const { t } = useI18n()
const temperature = computed(() => {
  const value = (props.device.runtime as { output?: Ds18b20TemperatureSensorOutputSnapshot }).output?.temperature as TemperatureOutputSnapshot | undefined
  return value
})
const temperatureText = computed(() => Ds18b20Device.formatTemperature(temperature.value) || t('device.dialog.temperatureUnavailableShort'))
</script>
