<template>
  <div class="d-flex flex-column ga-4">
    <div>
      <div v-if="device" class="d-flex flex-wrap ga-2 mb-3">
        <v-chip variant="tonal" :color="statusTone">{{ modeText }}</v-chip>
        <v-chip variant="tonal" :color="statusTone">{{ controlText }}</v-chip>
        <v-chip variant="outlined" :color="temperatureColor">{{ temperatureText }}</v-chip>
      </div>

      <v-alert v-if="device && statusTone !== 'primary'" :type="alertType" variant="tonal" class="mb-3">
        {{ statusText }}
      </v-alert>

      <v-alert v-if="sensorItems.length === 0" type="warning" variant="tonal" class="mb-3">
        {{ t('device.dialog.thermostat.noTemperatureSensor') }}
      </v-alert>
      <v-alert v-if="switchItems.length === 0" type="warning" variant="tonal" class="mb-3">
        {{ t('device.dialog.thermostat.noSwitch') }}
      </v-alert>

      <v-row>
        <v-col cols="12" sm="6">
          <v-select
            :label="t('device.fields.temperatureSensor')"
            :items="sensorItems"
            :model-value="modelValue.temperatureSensorDeviceId"
            :readonly="mode === 'view'"
            :disabled="(busy && mode !== 'view') || sensorItems.length === 0"
            @update:model-value="update('temperatureSensorDeviceId', Number($event))"
          />
        </v-col>
        <v-col cols="12" sm="6">
          <v-select
            :label="t('device.fields.switchDevice')"
            :items="switchItems"
            :model-value="modelValue.switchDeviceId"
            :readonly="mode === 'view'"
            :disabled="(busy && mode !== 'view') || switchItems.length === 0"
            @update:model-value="update('switchDeviceId', Number($event))"
          />
        </v-col>
      </v-row>
    </div>

    <v-row>
      <v-col cols="12" sm="4">
        <v-select
          :label="t('device.fields.mode')"
          :items="modeItems"
          :model-value="modelValue.mode"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('mode', $event as ThermostatMode)"
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
          step="0.1"
          :label="t('device.fields.minSafeCelsius')"
          :model-value="modelValue.minSafeCelsius"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('minSafeCelsius', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
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
    </v-row>

    <v-row>
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
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.minSwitchIntervalMs')"
          :model-value="modelValue.minSwitchIntervalMs"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('minSwitchIntervalMs', Number($event))"
        />
      </v-col>
      <v-col cols="12">
        <v-select
          :label="t('device.fields.algorithm')"
          :items="algorithmItems"
          :model-value="modelValue.algorithm"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('algorithm', $event as ThermostatAlgorithm)"
        />
      </v-col>
    </v-row>

    <v-row v-if="device">
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.desiredSwitchState')" :model-value="desiredSwitchText" readonly />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field :label="t('device.fields.actualSwitchState')" :model-value="actualSwitchText" readonly />
      </v-col>
    </v-row>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord, ThermostatOutputSnapshot } from '@/api/contracts'
import { dependencyOptionsForRole } from '@/models/devices/device-model-factory'
import {
  ThermostatDevice,
  type ThermostatAlgorithm,
  type ThermostatConfigDraft,
  type ThermostatMode,
} from '@/models/devices/thermostat'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{
  modelValue: ThermostatConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: ThermostatConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()

const sensorItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'temperature_sensor'))

const switchItems = computed(() => dependencyOptionsForRole(deviceStore.devices, 'switch'))

const modeItems = computed(() => (['off', 'heat', 'cool'] as ThermostatMode[]).map(value => ({ title: t(ThermostatDevice.modeLabelKey(value)), value })))
const algorithmItems = computed(() => (['hysteresis'] as ThermostatAlgorithm[]).map(value => ({ title: t(ThermostatDevice.algorithmLabelKey(value)), value })))

const output = computed(() => (props.device?.runtime as { output?: ThermostatOutputSnapshot } | undefined)?.output)
const temperature = computed(() => output.value?.temperature)
const temperatureText = computed(() => (props.device && temperature.value ? ThermostatDevice.formatOutput(temperature.value) || t('device.dialog.temperatureUnavailableShort') : ''))
const modeText = computed(() => t(ThermostatDevice.modeLabelKey(props.modelValue.mode)))
const statusText = computed(() => t(ThermostatDevice.statusLabelKey(output.value?.controlStatus ?? props.device?.runtime.effectiveStatus ?? props.device?.runtime.status ?? 'unknown')))
const controlText = computed(() => `${t('device.fields.controlStatus')}: ${statusText.value}`)
const desiredSwitchText = computed(() => t(`labels.output.${output.value?.desiredSwitchState ?? 'off'}`))
const actualSwitchText = computed(() => t(`labels.output.${output.value?.actualSwitchState ?? 'off'}`))
const statusTone = computed(() => ThermostatDevice.outputTone(props.device?.runtime.effectiveStatus ?? props.device?.runtime.status ?? 'unknown'))
const temperatureColor = computed(() => (temperature.value?.valid ? 'primary' : 'secondary'))
const alertType = computed(() => {
  if (statusTone.value === 'warning') return 'warning'
  if (statusTone.value === 'error') return 'error'
  if (statusTone.value === 'secondary') return 'info'
  return 'success'
})

function update<K extends keyof ThermostatConfigDraft>(key: K, value: ThermostatConfigDraft[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value })
}
</script>
