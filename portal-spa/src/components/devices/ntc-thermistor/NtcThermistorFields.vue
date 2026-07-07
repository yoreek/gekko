<template>
  <div class="d-flex flex-column ga-4">
    <v-row density="comfortable">
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.gpioPin')"
          :model-value="modelValue.gpioPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('gpioPin', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.ntcAttenuation')"
          :items="attenuationItems"
          :model-value="modelValue.attenuation"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('attenuation', $event as NtcAttenuation)"
        />
      </v-col>

      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.ntcSeriesResistorOhms')"
          :model-value="modelValue.seriesResistorOhms"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('seriesResistorOhms', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.ntcNominalResistanceOhms')"
          :model-value="modelValue.nominalResistanceOhms"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('nominalResistanceOhms', Number($event))"
        />
      </v-col>

      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          step="0.1"
          :label="t('device.fields.ntcNominalTempCelsius')"
          :model-value="modelValue.nominalTempCelsius"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('nominalTempCelsius', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.ntcBetaCoefficient')"
          :model-value="modelValue.betaCoefficient"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('betaCoefficient', Number($event))"
        />
      </v-col>

      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.ntcAdcSamples')"
          :model-value="modelValue.adcSamples"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('adcSamples', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.temperatureUnit')"
          :items="temperatureUnitItems"
          :model-value="modelValue.unit"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('unit', $event as TemperatureUnit)"
        />
      </v-col>

      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.pollMs')"
          :model-value="modelValue.pollMs"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('pollMs', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.reportDelta')"
          :model-value="modelValue.reportDeltaCelsius"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('reportDeltaCelsius', Number($event))"
        />
      </v-col>

      <v-col cols="12">
        <v-switch
          :label="t('device.fields.reportAlways')"
          :model-value="modelValue.reportAlways"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          inset
          @update:model-value="update('reportAlways', Boolean($event))"
        />
      </v-col>
    </v-row>

    <SensorFilterFields
      :model-value="modelValue.filter"
      :mode="mode"
      :busy="busy"
      @update:model-value="update('filter', $event)"
    />

    <v-row v-if="device" density="comfortable">
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.temperature')" :model-value="temperatureText" readonly />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.measuredAt')" :model-value="measuredAtText" readonly />
      </v-col>
    </v-row>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord, NtcThermistorTemperatureSensorOutputSnapshot, TemperatureOutputSnapshot, TemperatureUnit } from '@/api/contracts'
import { NtcThermistorDevice, type NtcAttenuation, type NtcThermistorConfigDraft } from '@/models/devices/ntc-thermistor'
import SensorFilterFields from '@/components/devices/common/SensorFilterFields.vue'

const props = defineProps<{
  modelValue: NtcThermistorConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: NtcThermistorConfigDraft]
}>()

const { t } = useI18n()

const attenuationItems = computed(() => NtcThermistorDevice.attenuationOptions.map(value => ({
  title: t(`device.dialog.ntcThermistor.attenuation.${value}`),
  value,
})))
const temperatureUnitItems = computed(() => NtcThermistorDevice.temperatureUnitOptions.map(value => ({ title: t(`device.dialog.temperatureUnit.${value}`), value })))

const output = computed(() => (props.device?.runtime as { output?: NtcThermistorTemperatureSensorOutputSnapshot } | undefined)?.output)
const temperature = computed(() => output.value?.temperature as TemperatureOutputSnapshot | undefined)
const temperatureText = computed(() => (temperature.value?.valid ? `${temperature.value.value.toFixed(2)} ${temperature.value.unitSymbol}` : t('device.dialog.temperatureUnavailableShort')))
const measuredAtText = computed(() => (temperature.value?.valid ? String(temperature.value.measuredAtMs) : ''))

function update<K extends keyof NtcThermistorConfigDraft>(key: K, value: NtcThermistorConfigDraft[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value })
}
</script>
