<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-alert
        v-if="statusTone !== 'primary'"
        :type="alertType"
        variant="tonal"
      >
        {{ statusText }}
      </v-alert>

      <div class="device-type-section__chips">
        <v-chip variant="tonal" :color="statusTone">
          {{ modeText }}
        </v-chip>
        <v-chip variant="tonal" :color="statusTone">
          {{ controlText }}
        </v-chip>
        <v-chip variant="outlined" :color="temperatureColor">
          {{ temperatureText }}
        </v-chip>
      </div>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="4">
          <v-text-field :label="t('device.fields.targetTemperature')" :model-value="targetTemperatureText" readonly />
        </v-col>
        <v-col cols="12" md="4">
          <v-text-field :label="t('device.fields.currentTemperature')" :model-value="temperatureText" readonly />
        </v-col>
      </v-row>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.temperatureSensor')" :model-value="sensorLabel" readonly />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.switchDevice')" :model-value="switchLabel" readonly />
        </v-col>
      </v-row>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="4">
          <v-text-field :label="t('device.fields.desiredSwitchState')" :model-value="desiredSwitchText" readonly />
        </v-col>
        <v-col cols="12" md="4">
          <v-text-field :label="t('device.fields.actualSwitchState')" :model-value="actualSwitchText" readonly />
        </v-col>
        <v-col cols="12" md="4">
          <v-text-field :label="t('device.fields.algorithm')" :model-value="algorithmText" readonly />
        </v-col>
      </v-row>
    </section>

    <section class="device-type-section">
      <v-expansion-panels>
        <v-expansion-panel value="details">
          <v-expansion-panel-title>
            {{ t('device.dialog.configDetails') }}
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <v-row class="device-type-section__grid">
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.mode')" :model-value="modeText" readonly />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.hysteresis')" :model-value="hysteresisText" readonly />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.checkIntervalMs')" :model-value="config.check_interval_ms" readonly />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.sensorTimeoutMs')" :model-value="config.sensor_timeout_ms" readonly />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.retryAfterErrorMs')" :model-value="config.retry_after_error_ms" readonly />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.minSwitchIntervalMs')" :model-value="config.min_switch_interval_ms" readonly />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.safeMinTemperature')" :model-value="minSafeTemperatureText" readonly />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.safeMaxTemperature')" :model-value="maxSafeTemperatureText" readonly />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.enabled')" :model-value="config.enabled ? t('labels.yes') : t('labels.no')" readonly />
              </v-col>
            </v-row>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DashboardDevice } from '@/models/device'
import {
  formatThermostatOutput,
  formatThermostatTemperature,
  normalizeThermostatConfig,
  thermostatAlgorithmLabelKey,
  thermostatModeLabelKey,
  thermostatOutputTone,
  thermostatStatusLabelKey,
} from '@/models/devices/thermostat'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{
  device: DashboardDevice
  busy?: boolean
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const config = computed(() => normalizeThermostatConfig(props.device.detail.config, props.device.deps))
const sensorDevice = computed(() => deviceStore.devices.find(device => device.deviceId === config.value.temperature_sensor_device_id))
const switchDevice = computed(() => deviceStore.devices.find(device => device.deviceId === config.value.switch_device_id))
const temperature = computed(() => props.device.output.temperature)
const temperatureText = computed(() => formatThermostatOutput(temperature.value) || t('device.dialog.temperatureUnavailableShort'))
const targetTemperatureText = computed(() => formatThermostatTemperature(config.value.target_celsius))
const minSafeTemperatureText = computed(() => formatThermostatTemperature(config.value.min_safe_celsius))
const maxSafeTemperatureText = computed(() => formatThermostatTemperature(config.value.max_safe_celsius))
const modeText = computed(() => t(thermostatModeLabelKey(config.value.mode)))
const algorithmText = computed(() => t(thermostatAlgorithmLabelKey(config.value.algorithm)))
const statusText = computed(() => t(thermostatStatusLabelKey(props.device.output.control_status ?? props.device.backendEffectiveStatus)))
const controlText = computed(() => `${t('device.fields.controlStatus')}: ${statusText.value}`)
const desiredSwitchText = computed(() => t(`labels.output.${props.device.output.desired_switch_state ?? 'off'}`))
const actualSwitchText = computed(() => t(`labels.output.${props.device.output.actual_switch_state ?? 'off'}`))
const sensorLabel = computed(() =>
  sensorDevice.value ? `${sensorDevice.value.name} #${sensorDevice.value.deviceId}` : `#${config.value.temperature_sensor_device_id || '—'}`,
)
const switchLabel = computed(() =>
  switchDevice.value ? `${switchDevice.value.name} #${switchDevice.value.deviceId}` : `#${config.value.switch_device_id || '—'}`,
)
const statusTone = computed(() => thermostatOutputTone(props.device.output.control_status ?? props.device.backendEffectiveStatus))
const temperatureColor = computed(() => (temperature.value?.valid ? 'primary' : 'secondary'))
const alertType = computed(() => {
  if (statusTone.value === 'warning') {
    return 'warning'
  }
  if (statusTone.value === 'error') {
    return 'error'
  }
  if (statusTone.value === 'secondary') {
    return 'info'
  }
  return 'success'
})
const hysteresisText = computed(() => `${config.value.hysteresis_celsius.toFixed(1)}°C`)
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

.device-type-section__chips {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.device-type-section__grid {
  margin: 0;
}

.device-type-section :deep(.v-expansion-panel-text__wrapper) {
  padding: 8px 0 0;
}
</style>
