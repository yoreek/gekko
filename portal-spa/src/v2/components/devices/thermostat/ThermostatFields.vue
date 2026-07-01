<template>
  <v-row>
    <v-col cols="12" sm="6">
      <v-select
        :label="t('device.fields.mode')"
        :items="modeItems"
        :model-value="modelValue.mode"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('mode', $event as ThermostatMode)"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-select
        :label="t('device.fields.algorithm')"
        :items="[{ title: 'Hysteresis', value: 'hysteresis' }]"
        :model-value="modelValue.algorithm"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
      />
    </v-col>

    <v-col cols="12" sm="4">
      <v-text-field
        type="number"
        step="0.1"
        :label="t('device.fields.targetCelsius')"
        :model-value="modelValue.targetCelsius"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('targetCelsius', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="4">
      <v-text-field
        type="number"
        step="0.1"
        :label="t('device.fields.minSafeCelsius')"
        :model-value="modelValue.minSafeCelsius"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('minSafeCelsius', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="4">
      <v-text-field
        type="number"
        step="0.1"
        :label="t('device.fields.maxSafeCelsius')"
        :model-value="modelValue.maxSafeCelsius"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('maxSafeCelsius', Number($event))"
      />
    </v-col>

    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        step="0.1"
        :label="t('device.fields.hysteresisCelsius')"
        :model-value="modelValue.hysteresisCelsius"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('hysteresisCelsius', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        :label="t('device.fields.checkIntervalMs')"
        :model-value="modelValue.checkIntervalMs"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('checkIntervalMs', Number($event))"
      />
    </v-col>

    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        :label="t('device.fields.sensorTimeoutMs')"
        :model-value="modelValue.sensorTimeoutMs"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('sensorTimeoutMs', Number($event))"
      />
    </v-col>
    <v-col cols="12" sm="6">
      <v-text-field
        type="number"
        :label="t('device.fields.retryAfterErrorMs')"
        :model-value="modelValue.retryAfterErrorMs"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('retryAfterErrorMs', Number($event))"
      />
    </v-col>

    <v-col cols="12">
      <v-text-field
        type="number"
        :label="t('device.fields.minSwitchIntervalMs')"
        :model-value="modelValue.minSwitchIntervalMs"
        :readonly="mode === 'view'"
        :disabled="busy && mode !== 'view'"
        @update:model-value="update('minSwitchIntervalMs', Number($event))"
      />
    </v-col>
  </v-row>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'
import type { ThermostatConfigDraft, ThermostatMode } from '@/models/devices/thermostat'

const props = defineProps<{
  modelValue: ThermostatConfigDraft
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: ThermostatConfigDraft]
}>()

const { t } = useI18n()

const modeItems = computed(() => [
  { title: 'Off', value: 'off' },
  { title: 'Heat', value: 'heat' },
  { title: 'Cool', value: 'cool' },
])

function update<K extends keyof ThermostatConfigDraft>(key: K, value: ThermostatConfigDraft[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value })
}
</script>
