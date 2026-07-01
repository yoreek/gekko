<template>
  <v-row>
    <v-col cols="12">
      <v-text-field
        :label="t('device.fields.address')"
        :model-value="modelValue.address"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        hint="OneWire device address in hex (e.g., 28XXXXXXXXXXXX)"
        persistent-hint
        @update:model-value="update('address', String($event).toUpperCase())"
      />
    </v-col>

    <v-col cols="12" sm="6">
      <v-select
        :label="t('device.fields.resolution')"
        :items="[9, 10, 11, 12]"
        :model-value="modelValue.resolution"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('resolution', Number($event) as 9 | 10 | 11 | 12)"
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
        :label="t('device.fields.reportDeltaCelsius')"
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
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { TemperatureUnit } from '@/api/contracts'
import type { Ds18b20ConfigDraft } from '@/models/devices/ds18b20'

const props = defineProps<{
  modelValue: Ds18b20ConfigDraft
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: Ds18b20ConfigDraft]
}>()

const { t } = useI18n()

const temperatureUnitItems = computed(() => [
  { title: 'Celsius', value: 'celsius' },
  { title: 'Fahrenheit', value: 'fahrenheit' },
])

function update<K extends keyof Ds18b20ConfigDraft>(key: K, value: Ds18b20ConfigDraft[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value })
}
</script>
