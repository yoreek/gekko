<template>
  <div class="d-flex flex-column ga-4">
    <v-row>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.gpioPin')"
          :hint="t('device.dialog.gpioPinHint')"
          :model-value="modelValue.gpioPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('gpioPin', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-switch
          :label="t('device.fields.internalPullup')"
          :model-value="modelValue.internalPullup"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          inset
          @update:model-value="update('internalPullup', Boolean($event))"
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
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.reportDeltaHumidity')"
          :model-value="modelValue.reportDeltaHumidity"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('reportDeltaHumidity', Number($event))"
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
      :model-value="modelValue.temperatureFilter"
      :mode="mode"
      :busy="busy"
      title-key="device.dialog.aht10.temperatureFilterTitle"
      :current-reading="temperature?.valid ? temperature.value : undefined"
      :reading-unit="temperature?.unitSymbol"
      @update:model-value="update('temperatureFilter', $event)"
    />

    <SensorFilterFields
      :model-value="modelValue.humidityFilter"
      :mode="mode"
      :busy="busy"
      title-key="device.dialog.aht10.humidityFilterTitle"
      :current-reading="humidity?.valid ? humidity.value : undefined"
      :reading-unit="humidity?.unitSymbol"
      @update:model-value="update('humidityFilter', $event)"
    />

    <v-row v-if="device">
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.temperature')" :model-value="temperatureText" readonly />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.humidity')" :model-value="humidityText" readonly />
      </v-col>
    </v-row>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord, Dht11SensorOutputSnapshot, TemperatureUnit } from '@/api/contracts'
import { Dht11Device, type Dht11ConfigDraft } from '@/models/devices/dht11'
import SensorFilterFields from '@/components/devices/common/SensorFilterFields.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: Dht11ConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Dht11ConfigDraft]
}>()

const { t } = useI18n()

const temperatureUnitItems = computed(() =>
  Dht11Device.temperatureUnitOptions.map(value => ({ title: t(`device.dialog.temperatureUnit.${value}`), value })),
)

const output = computed(() => (props.device?.runtime as { output?: Dht11SensorOutputSnapshot } | undefined)?.output)
const temperature = computed(() => output.value?.temperature)
const humidity = computed(() => output.value?.humidity)
const temperatureText = computed(() =>
  output.value?.temperature?.valid
    ? Dht11Device.formatTemperature(output.value.temperature)
    : t('device.dialog.temperatureUnavailableShort'),
)
const humidityText = computed(() =>
  output.value?.humidity?.valid ? Dht11Device.formatHumidity(output.value.humidity) : t('device.dialog.temperatureUnavailableShort'),
)

const { update } = useDraftModel(props, emit)
</script>
