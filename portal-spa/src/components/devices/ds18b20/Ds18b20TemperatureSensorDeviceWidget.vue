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
import { Ds18b20 } from '@/models/devices/ds18b20'

const deviceModel = new Ds18b20.Device()

const props = defineProps<{
  device: DashboardDevice
  editable?: boolean
}>()

defineEmits<{
  open: []
}>()

const { t } = useI18n()
const temperature = computed(() => {
  const value = deviceModel.normalizeOutput(props.device.raw).temperature
  return Ds18b20.temperatureValid(value) ? value : undefined
})
const temperatureText = computed(() => Ds18b20.formatTemperature(temperature.value) || t('device.dialog.temperatureUnavailableShort'))
</script>
