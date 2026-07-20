<template>
  <div class="d-flex flex-column ga-4">
    <div class="text-label-small text-medium-emphasis">{{ t('device.dialog.cd74hc4067Hub.selectPinsTitle') }}</div>
    <v-row density="comfortable">
      <v-col v-for="(pin, index) in modelValue.selectPins" :key="index" cols="6" sm="3">
        <v-text-field
          type="number"
          :label="`S${index}`"
          :model-value="pin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="updateSelectPin(index, Number($event))"
        />
      </v-col>
    </v-row>

    <v-row density="comfortable">
      <v-col cols="12" sm="4">
        <v-text-field
          type="number"
          :label="t('device.fields.cd74hc4067EnablePin')"
          :hint="t('device.dialog.cd74hc4067Hub.enablePinHint')"
          persistent-hint
          :model-value="modelValue.enablePin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('enablePin', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="4">
        <v-text-field
          type="number"
          :label="t('device.fields.cd74hc4067SigPin')"
          :model-value="modelValue.sigPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('sigPin', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="4">
        <v-select
          :label="t('device.fields.ntcAttenuation')"
          :items="attenuationItems"
          :model-value="modelValue.sigAttenuation"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('sigAttenuation', $event as AdcAttenuation)"
        />
      </v-col>
    </v-row>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord } from '@/api/contracts'
import type { Cd74hc4067HubConfigDraft } from '@/models/devices/cd74hc4067-hub'
import type { AdcAttenuation } from '@/models/devices/analog-port-input'
import { useDraftModel } from '@/composables/useDraftModel'

const props = defineProps<{
  modelValue: Cd74hc4067HubConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Cd74hc4067HubConfigDraft]
}>()

const { t } = useI18n()

const attenuationItems = computed(() => (['0db', '2_5db', '6db', '11db'] as AdcAttenuation[]).map(value => ({
  title: t(`device.dialog.ntcThermistor.attenuation.${value}`),
  value,
})))

const { update } = useDraftModel(props, emit)

function updateSelectPin(index: number, value: number): void {
  const selectPins = [...props.modelValue.selectPins] as [number, number, number, number]
  selectPins[index] = value
  update('selectPins', selectPins)
}
</script>
