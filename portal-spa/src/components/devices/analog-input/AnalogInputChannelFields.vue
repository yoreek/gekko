<template>
  <div class="d-flex flex-column ga-4">
    <v-alert v-if="dependencyItems.length === 0" type="warning" variant="tonal">
      {{ t('device.dialog.analogInputChannel.noDependency') }}
    </v-alert>

    <v-row density="comfortable">
      <v-col cols="12" sm="6">
        <v-select
          :label="t('device.fields.analogInputHubDeviceId')"
          :items="dependencyItems"
          :model-value="modelValue.dependencyDeviceId"
          :readonly="mode === 'view'"
          :disabled="(busy && mode !== 'view') || dependencyItems.length === 0"
          @update:model-value="update('dependencyDeviceId', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :min="0"
          :max="maxChannel"
          :label="t('device.fields.analogInputChannel')"
          :model-value="modelValue.channel"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('channel', Number($event))"
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
import type { AnalogInputChannelConfigDraft } from '@/models/devices/analog-input-channel'
import { analogInputHubChannelCount } from '@/models/devices/analog-input-channel'
import { dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: AnalogInputChannelConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: AnalogInputChannelConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const dependencyItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'analog_input_hub'))
const selectedHub = computed(() => deviceStore.devices.find(device => device.record.id === props.modelValue.dependencyDeviceId))
const maxChannel = computed(() => analogInputHubChannelCount(selectedHub.value?.record.typeName) - 1)

const output = computed(() => (props.device?.runtime as { output?: AnalogInputOutputSnapshot } | undefined)?.output)
const reading = computed(() => output.value?.analogInput)
const voltageText = computed(() => (reading.value?.valid ? `${((reading.value.milliVolts ?? 0) / 1000).toFixed(3)} V` : t('device.dialog.temperatureUnavailableShort')))
const measuredAtText = computed(() => (reading.value?.valid ? String(reading.value.measuredAtMs) : ''))

const { update } = useDraftModel(props, emit)
</script>
