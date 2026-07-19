<template>
  <div class="d-flex flex-column ga-4">
    <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
      {{ t('device.dialog.htu21.noDependency') }}
    </v-alert>

    <v-select
      :label="t('device.fields.i2cBusDeviceId')"
      :items="dependencyItems"
      :model-value="modelValue.dependencyDeviceId"
      :readonly="mode === 'view'"
      :disabled="(busy && mode !== 'view') || dependencyItems.length === 0"
      @update:model-value="update('dependencyDeviceId', Number($event))"
    />

    <I2cAddressPicker
      :model-value="modelValue.i2cAddress"
      :bus-device-id="modelValue.dependencyDeviceId"
      @update:model-value="update('i2cAddress', $event)"
    />

    <v-alert type="info" variant="tonal">
      {{ t('device.dialog.htu21.addressHint') }}
    </v-alert>

    <v-row>
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
      title-key="device.dialog.htu21.temperatureFilterTitle"
      @update:model-value="update('temperatureFilter', $event)"
    />

    <SensorFilterFields
      :model-value="modelValue.humidityFilter"
      :mode="mode"
      :busy="busy"
      title-key="device.dialog.htu21.humidityFilterTitle"
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

import type { DeviceRecord, Htu21SensorOutputSnapshot, TemperatureUnit } from '@/api/contracts'
import { Htu21Device, type Htu21ConfigDraft } from '@/models/devices/htu21'
import { dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import SensorFilterFields from '@/components/devices/common/SensorFilterFields.vue'
import I2cAddressPicker from '@/components/devices/common/I2cAddressPicker.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: Htu21ConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Htu21ConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const dependencyItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'i2c_bus'))
const temperatureUnitItems = computed(() =>
  Htu21Device.temperatureUnitOptions.map(value => ({ title: t(`device.dialog.temperatureUnit.${value}`), value })),
)

const output = computed(() => (props.device?.runtime as { output?: Htu21SensorOutputSnapshot } | undefined)?.output)
const temperatureText = computed(() =>
  output.value?.temperature?.valid
    ? Htu21Device.formatTemperature(output.value.temperature)
    : t('device.dialog.temperatureUnavailableShort'),
)
const humidityText = computed(() =>
  output.value?.humidity?.valid ? Htu21Device.formatHumidity(output.value.humidity) : t('device.dialog.temperatureUnavailableShort'),
)

const { update } = useDraftModel(props, emit)
</script>
