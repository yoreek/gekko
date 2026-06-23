<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-alert v-if="sensorItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.thermostat.noTemperatureSensor') }}
      </v-alert>
      <v-alert v-if="switchItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.thermostat.noSwitch') }}
      </v-alert>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.temperatureSensor')"
            :items="sensorItems"
            :model-value="currentValue.temperature_sensor_device_id"
            :disabled="busy || sensorItems.length === 0"
            :rules="sensorRules"
            @update:model-value="updateNumber('temperature_sensor_device_id', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.switchDevice')"
            :items="switchItems"
            :model-value="currentValue.switch_device_id"
            :disabled="busy || switchItems.length === 0"
            :rules="switchRules"
            @update:model-value="updateNumber('switch_device_id', $event)"
          />
        </v-col>
      </v-row>
    </section>

    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="4">
          <v-select
            :label="t('device.fields.mode')"
            :items="modeItems"
            :model-value="currentValue.mode"
            :disabled="busy"
            @update:model-value="update('mode', $event as Thermostat.Mode)"
          />
        </v-col>
        <v-col cols="12" md="4">
          <v-text-field
            :label="t('device.fields.targetTemperature')"
            :model-value="currentValue.target_celsius"
            inputmode="decimal"
            type="number"
            step="0.1"
            :disabled="busy"
            @update:model-value="updateNumber('target_celsius', $event)"
          />
        </v-col>
        <v-col cols="12" md="4">
          <v-text-field
            :label="t('device.fields.hysteresis')"
            :model-value="currentValue.hysteresis_celsius"
            inputmode="decimal"
            type="number"
            step="0.1"
            min="0"
            :disabled="busy"
            @update:model-value="updateNumber('hysteresis_celsius', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.safeMinTemperature')"
            :model-value="currentValue.min_safe_celsius"
            inputmode="decimal"
            type="number"
            step="0.1"
            :disabled="busy"
            @update:model-value="updateNumber('min_safe_celsius', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            :label="t('device.fields.safeMaxTemperature')"
            :model-value="currentValue.max_safe_celsius"
            inputmode="decimal"
            type="number"
            step="0.1"
            :disabled="busy"
            @update:model-value="updateNumber('max_safe_celsius', $event)"
          />
        </v-col>
      </v-row>
    </section>

    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="3">
          <v-text-field
            :label="t('device.fields.checkIntervalMs')"
            :model-value="currentValue.check_interval_ms"
            inputmode="numeric"
            type="number"
            min="100"
            step="100"
            :disabled="busy"
            @update:model-value="updateNumber('check_interval_ms', $event)"
          />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field
            :label="t('device.fields.sensorTimeoutMs')"
            :model-value="currentValue.sensor_timeout_ms"
            inputmode="numeric"
            type="number"
            min="100"
            step="100"
            :disabled="busy"
            @update:model-value="updateNumber('sensor_timeout_ms', $event)"
          />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field
            :label="t('device.fields.retryAfterErrorMs')"
            :model-value="currentValue.retry_after_error_ms"
            inputmode="numeric"
            type="number"
            min="100"
            step="100"
            :disabled="busy"
            @update:model-value="updateNumber('retry_after_error_ms', $event)"
          />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field
            :label="t('device.fields.minSwitchIntervalMs')"
            :model-value="currentValue.min_switch_interval_ms"
            inputmode="numeric"
            type="number"
            min="0"
            step="100"
            :disabled="busy"
            @update:model-value="updateNumber('min_switch_interval_ms', $event)"
          />
        </v-col>
        <v-col cols="12">
          <v-select
            :label="t('device.fields.algorithm')"
            :items="algorithmItems"
            :model-value="currentValue.algorithm"
            :disabled="busy"
            @update:model-value="update('algorithm', $event as Thermostat.Algorithm)"
          />
        </v-col>
      </v-row>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import { Thermostat } from '@/models/devices/thermostat'
import {
  DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
  GPIO_SWITCH_DEVICE_TYPE_ID,
  THERMOSTAT_DEVICE_TYPE_ID,
  resolveDeviceTypeOption,
} from '@/models/device-types'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

type ThermostatFormValue = Thermostat.CreateDraft | Thermostat.ConfigDraft

const props = defineProps<{
  modelValue: ThermostatFormValue | undefined
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: ThermostatFormValue]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const isCreateMode = computed(() => props.mode !== 'edit')
const fallbackValue: ThermostatFormValue = {
  name: 'New Device',
  typeId: THERMOSTAT_DEVICE_TYPE_ID,
  enabled: true,
  mode: 'heat',
  algorithm: 'hysteresis',
  target_celsius: 25,
  min_safe_celsius: 0,
  max_safe_celsius: 50,
  hysteresis_celsius: 0.5,
  check_interval_ms: 1000,
  sensor_timeout_ms: 6000,
  retry_after_error_ms: 30000,
  min_switch_interval_ms: 5000,
  temperature_sensor_device_id: 0,
  switch_device_id: 0,
}
const currentValue = computed<ThermostatFormValue>(() => props.modelValue ?? fallbackValue)
const sensorItems = computed(() =>
  deviceStore.devices
    .filter(device => device.typeId === DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID)
    .map(device => ({
      title: `${device.name} #${device.deviceId}`,
      value: device.deviceId,
    })),
)
const switchItems = computed(() =>
  deviceStore.devices
    .filter(device => {
      const option = resolveDeviceTypeOption(device.typeId)
      return device.typeId === GPIO_SWITCH_DEVICE_TYPE_ID || (option?.supportedOutputStates?.includes('off') && option?.supportedOutputStates?.includes('on'))
    })
    .map(device => ({
      title: `${device.name} #${device.deviceId}`,
      value: device.deviceId,
    })),
)
const modeItems = computed(() => ['off', 'heat', 'cool'].map(value => ({ title: t(Thermostat.modeLabelKey(value as Thermostat.Mode)), value })))
const algorithmItems = computed(() =>
  ['hysteresis'].map(value => ({ title: t(Thermostat.algorithmLabelKey(value as Thermostat.Algorithm)), value })),
)
const sensorRules = computed(() => [
  (value: unknown) => Number(value) > 0 || t('device.dialog.thermostat.noTemperatureSensor'),
])
const switchRules = computed(() => [
  (value: unknown) => Number(value) > 0 || t('device.dialog.thermostat.noSwitch'),
])

function emitUpdate(next: ThermostatFormValue): void {
  emit('update:modelValue', next)
}

function update<K extends keyof Thermostat.CreateDraft>(key: K, value: Thermostat.CreateDraft[K]): void {
  emitUpdate(buildNextValue({ [key]: value } as Partial<Thermostat.CreateDraft>))
}

function updateNumber(key: keyof Pick<
  Thermostat.CreateDraft,
  | 'temperature_sensor_device_id'
  | 'switch_device_id'
  | 'target_celsius'
  | 'min_safe_celsius'
  | 'max_safe_celsius'
  | 'hysteresis_celsius'
  | 'check_interval_ms'
  | 'sensor_timeout_ms'
  | 'retry_after_error_ms'
  | 'min_switch_interval_ms'
>, value: unknown): void {
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  update(key as never, numeric as never)
}

function buildNextValue(patch: Partial<Thermostat.CreateDraft>): ThermostatFormValue {
  if (!isCreateMode.value) {
    return {
      ...(currentValue.value as Thermostat.CreateDraft),
      ...patch,
    }
  }
  return {
    ...(currentValue.value as Thermostat.CreateDraft),
    ...patch,
  }
}
</script>

<style scoped>
.device-type-stack {
  display: grid;
  gap: 12px;
}

.device-type-section {
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid rgb(var(--v-theme-outline-variant));
  border-radius: 10px;
  background: var(--portal-surface);
  box-shadow: var(--portal-shadow-sm);
}

.device-type-section__grid {
  margin: 0;
}

.device-type-section :deep(.v-expansion-panel-text__wrapper) {
  padding: 8px 0 0;
}
</style>
