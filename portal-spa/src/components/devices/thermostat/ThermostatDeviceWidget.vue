<template>
  <DeviceWidgetBase :device="device" :editable="editable" @open="$emit('open')">
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

import type { DeviceRecord, ThermostatOutputSnapshot } from '@/api/contracts'
import DeviceWidgetBase from '@/components/devices/base/DeviceWidgetBase.vue'
import { Thermostat } from '@/models/devices/thermostat'

const props = defineProps<{
  device: DeviceRecord
  editable?: boolean
}>()

defineEmits<{
  open: []
}>()

const { t } = useI18n()

const config = computed(() => Thermostat.normalizeConfig(props.device.config, props.device.config.deps))
const output = computed(() => (props.device.runtime as { output?: ThermostatOutputSnapshot }).output)
const temperature = computed(() => output.value?.temperature)
const currentTemperatureText = computed(() => Thermostat.formatOutput(temperature.value) || t('device.dialog.temperatureUnavailableShort'))
const summaryTone = computed(() => Thermostat.outputTone(props.device.runtime.effectiveStatus ?? props.device.runtime.lifecycleStatus ?? props.device.runtime.status ?? 'unknown'))
const summaryText = computed(() => {
  const mode = t(Thermostat.modeLabelKey(config.value.mode))
  return `${mode} ${Thermostat.formatOutput(temperature.value) || formatTemperatureSetpoint(config.value.targetCelsius)}`
})
const summaryTitle = computed(() =>
  [
    t(Thermostat.modeLabelKey(config.value.mode)),
    `${t('device.fields.targetTemperature')}: ${formatTemperatureSetpoint(config.value.targetCelsius)}`,
    `${t('device.fields.currentTemperature')}: ${currentTemperatureText.value}`,
    `${t('device.fields.controlStatus')}: ${t(Thermostat.statusLabelKey(output.value?.controlStatus ?? props.device.runtime.effectiveStatus ?? props.device.runtime.lifecycleStatus ?? props.device.runtime.status ?? 'unknown'))}`,
  ].join(' · '),
)

function formatTemperatureSetpoint(value: number): string {
  return `${value.toFixed(1)}°C`
}
</script>
