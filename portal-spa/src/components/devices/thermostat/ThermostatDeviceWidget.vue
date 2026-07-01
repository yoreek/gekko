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
import { ThermostatDevice } from '@/models/devices/thermostat'

const props = defineProps<{
  device: DeviceRecord
  editable?: boolean
}>()

defineEmits<{
  open: []
}>()

const { t } = useI18n()

const config = computed(() => ThermostatDevice.normalizeConfig(props.device.config, props.device.config.deps))
const output = computed(() => (props.device.runtime as { output?: ThermostatOutputSnapshot }).output)
const temperature = computed(() => output.value?.temperature)
const currentTemperatureText = computed(() => ThermostatDevice.formatOutput(temperature.value) || t('device.dialog.temperatureUnavailableShort'))
const summaryTone = computed(() => ThermostatDevice.outputTone(props.device.runtime.effectiveStatus ?? props.device.runtime.lifecycleStatus ?? props.device.runtime.status ?? 'unknown'))
const summaryText = computed(() => {
  const mode = t(ThermostatDevice.modeLabelKey(config.value.mode))
  return `${mode} ${ThermostatDevice.formatOutput(temperature.value) || formatTemperatureSetpoint(config.value.targetCelsius)}`
})
const summaryTitle = computed(() =>
  [
    t(ThermostatDevice.modeLabelKey(config.value.mode)),
    `${t('device.fields.targetTemperature')}: ${formatTemperatureSetpoint(config.value.targetCelsius)}`,
    `${t('device.fields.currentTemperature')}: ${currentTemperatureText.value}`,
    `${t('device.fields.controlStatus')}: ${t(ThermostatDevice.statusLabelKey(output.value?.controlStatus ?? props.device.runtime.effectiveStatus ?? props.device.runtime.lifecycleStatus ?? props.device.runtime.status ?? 'unknown'))}`,
  ].join(' · '),
)

function formatTemperatureSetpoint(value: number): string {
  return `${value.toFixed(1)}°C`
}
</script>
