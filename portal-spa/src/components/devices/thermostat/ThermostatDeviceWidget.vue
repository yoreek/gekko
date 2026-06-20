<template>
  <DeviceWidgetBase :device="device" :editable="editable" @open="$emit('open')" @remove="$emit('remove')">
    <template #actions>
      <v-chip size="small" variant="tonal" :color="summaryTone" :title="summaryTitle">
        {{ summaryText }}
      </v-chip>
    </template>
  </DeviceWidgetBase>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import DeviceWidgetBase from '@/components/devices/base/DeviceWidgetBase.vue'
import type { DashboardDevice } from '@/models/device'
import {
  formatThermostatOutput,
  normalizeThermostatConfig,
  thermostatModeLabelKey,
  thermostatOutputTone,
  thermostatStatusLabelKey,
} from '@/models/devices/thermostat'

const props = defineProps<{
  device: DashboardDevice
  editable?: boolean
}>()

defineEmits<{
  open: []
  remove: []
}>()

const { t } = useI18n()

const config = computed(() => normalizeThermostatConfig(props.device.detail.config, props.device.deps))
const temperature = computed(() => props.device.output.temperature)
const currentTemperatureText = computed(() => formatThermostatOutput(temperature.value) || t('device.dialog.temperatureUnavailableShort'))
const summaryTone = computed(() => thermostatOutputTone(props.device.backendEffectiveStatus))
const summaryText = computed(() => {
  const mode = t(thermostatModeLabelKey(config.value.mode))
  return `${mode} ${formatThermostatOutput(temperature.value) || formatTemperatureSetpoint(config.value.target_celsius)}`
})
const summaryTitle = computed(() =>
  [
    t(thermostatModeLabelKey(config.value.mode)),
    `${t('device.fields.targetTemperature')}: ${formatTemperatureSetpoint(config.value.target_celsius)}`,
    `${t('device.fields.currentTemperature')}: ${currentTemperatureText.value}`,
    `${t('device.fields.controlStatus')}: ${t(thermostatStatusLabelKey(props.device.output.control_status ?? props.device.backendEffectiveStatus))}`,
  ].join(' · '),
)

function formatTemperatureSetpoint(value: number): string {
  return `${value.toFixed(1)}°C`
}
</script>
