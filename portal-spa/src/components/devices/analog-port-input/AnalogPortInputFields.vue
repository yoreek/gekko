<template>
  <div class="d-flex flex-column ga-4">
    <v-row density="comfortable">
      <v-col cols="12" sm="6">
        <PinPicker
          :current-device-id="device?.record.id"
          :label="t('device.fields.gpioPin')"
          required-role="adc1"
          :model-value="modelValue.gpioPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('gpioPin', $event)"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.ntcAttenuation')"
          :items="attenuationItems"
          :model-value="modelValue.attenuation"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('attenuation', $event as AdcAttenuation)"
        />
      </v-col>

      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.adcSamples')"
          :model-value="modelValue.adcSamples"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('adcSamples', Number($event))"
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
          :label="t('device.fields.reportDeltaMilliVolts')"
          :model-value="modelValue.reportDeltaMilliVolts"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('reportDeltaMilliVolts', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6" class="d-flex align-center">
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

    <v-row v-if="device" density="comfortable">
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.analogInputVoltage')" :model-value="voltageText" readonly />
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

import type { AnalogInputOutputSnapshot, DeviceRecord } from '@/api/contracts'
import { AnalogPortInputDevice, type AdcAttenuation, type AnalogPortInputConfigDraft } from '@/models/devices/analog-port-input'
import PinPicker from '@/components/devices/common/PinPicker.vue'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: AnalogPortInputConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: AnalogPortInputConfigDraft]
}>()

const { t } = useI18n()

const attenuationItems = computed(() => AnalogPortInputDevice.attenuationOptions.map(value => ({
  title: t(`device.dialog.ntcThermistor.attenuation.${value}`),
  value,
})))

const output = computed(() => (props.device?.runtime as { output?: AnalogInputOutputSnapshot } | undefined)?.output)
const reading = computed(() => output.value?.analogInput)
const voltageText = computed(() => (reading.value?.valid ? `${((reading.value.milliVolts ?? 0) / 1000).toFixed(3)} V` : t('device.dialog.temperatureUnavailableShort')))
const measuredAtText = computed(() => (reading.value?.valid ? String(reading.value.measuredAtMs) : ''))

const { update } = useDraftModel(props, emit)
</script>
