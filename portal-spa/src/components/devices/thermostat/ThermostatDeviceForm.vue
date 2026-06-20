<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-alert v-if="sensorItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.thermostatNoTemperatureSensor') }}
      </v-alert>
      <v-alert v-if="switchItems.length === 0" type="warning" variant="tonal">
        {{ t('device.dialog.thermostatNoSwitch') }}
      </v-alert>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.temperatureSensor')"
            :items="sensorItems"
            :model-value="currentValue.temperature_sensor_device_id"
            :disabled="busy || sensorItems.length === 0"
            @update:model-value="updateNumber('temperature_sensor_device_id', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-select
            :label="t('device.fields.switchDevice')"
            :items="switchItems"
            :model-value="currentValue.switch_device_id"
            :disabled="busy || switchItems.length === 0"
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
            @update:model-value="update('mode', $event as ThermostatMode)"
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
            @update:model-value="update('algorithm', $event as ThermostatAlgorithm)"
          />
        </v-col>
      </v-row>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import {
  createDefaultThermostatConfig,
  thermostatAlgorithmLabelKey,
  thermostatModeLabelKey,
  type ThermostatAlgorithm,
  type ThermostatConfigDraft,
  type ThermostatMode,
} from '@/models/devices/thermostat'
import {
  DS18B20_TEMPERATURE_SENSOR_DEVICE_TYPE_ID,
  GPIO_SWITCH_DEVICE_TYPE_ID,
  resolveDeviceTypeOption,
} from '@/models/device-types'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{
  modelValue: ThermostatConfigDraft | undefined
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: ThermostatConfigDraft]
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const fallbackValue = createDefaultThermostatConfig()
const currentValue = computed<ThermostatConfigDraft>(() => props.modelValue ?? fallbackValue)
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
const modeItems = computed(() => ['off', 'heat', 'cool'].map(value => ({ title: t(thermostatModeLabelKey(value as ThermostatMode)), value })))
const algorithmItems = computed(() =>
  ['hysteresis'].map(value => ({ title: t(thermostatAlgorithmLabelKey(value as ThermostatAlgorithm)), value })),
)

function emitUpdate(next: ThermostatConfigDraft): void {
  emit('update:modelValue', next)
}

function update<K extends keyof ThermostatConfigDraft>(key: K, value: ThermostatConfigDraft[K]): void {
  emitUpdate({
    ...currentValue.value,
    [key]: value,
  })
}

function updateNumber(key: keyof Pick<
  ThermostatConfigDraft,
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
