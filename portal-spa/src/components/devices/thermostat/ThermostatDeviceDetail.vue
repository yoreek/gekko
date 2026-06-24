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
                <v-text-field :label="t('device.fields.checkIntervalMs')" :model-value="config.checkIntervalMs" readonly />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.sensorTimeoutMs')" :model-value="config.sensorTimeoutMs" readonly />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.retryAfterErrorMs')" :model-value="config.retryAfterErrorMs" readonly />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field :label="t('device.fields.minSwitchIntervalMs')" :model-value="config.minSwitchIntervalMs" readonly />
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

import type { DeviceRecord, ThermostatOutputSnapshot } from '@/api/contracts'
import { deviceRecordConfig, deviceRecordEffectiveStatus, deviceRecordId, deviceRecordName, deviceRecordRuntime } from '@/models/device'
import { Thermostat } from '@/models/devices/thermostat'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const deviceModel = new Thermostat.Device()

const props = defineProps<{
  device: DeviceRecord
  busy?: boolean
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const config = computed(() => deviceModel.normalizeConfig(deviceRecordConfig(props.device), deviceRecordConfig(props.device).deps))
const output = computed(() => deviceRecordRuntime(props.device) as ThermostatOutputSnapshot)
const sensorDevice = computed(() => deviceStore.devices.find(device => device.record.id === config.value.temperatureSensorDeviceId))
const switchDevice = computed(() => deviceStore.devices.find(device => device.record.id === config.value.switchDeviceId))
const temperature = computed(() => output.value.temperature)
const temperatureText = computed(() => Thermostat.formatOutput(temperature.value) || t('device.dialog.temperatureUnavailableShort'))
const targetTemperatureText = computed(() => Thermostat.formatTemperature(config.value.targetCelsius))
const minSafeTemperatureText = computed(() => Thermostat.formatTemperature(config.value.minSafeCelsius))
const maxSafeTemperatureText = computed(() => Thermostat.formatTemperature(config.value.maxSafeCelsius))
const modeText = computed(() => t(Thermostat.modeLabelKey(config.value.mode)))
const algorithmText = computed(() => t(Thermostat.algorithmLabelKey(config.value.algorithm)))
const statusText = computed(() => t(Thermostat.statusLabelKey(output.value.controlStatus ?? deviceRecordEffectiveStatus(props.device))))
const controlText = computed(() => `${t('device.fields.controlStatus')}: ${statusText.value}`)
const desiredSwitchText = computed(() => t(`labels.output.${output.value.desiredSwitchState ?? 'off'}`))
const actualSwitchText = computed(() => t(`labels.output.${output.value.actualSwitchState ?? 'off'}`))
const sensorLabel = computed(() =>
  sensorDevice.value ? `${deviceRecordName(sensorDevice.value)} #${deviceRecordId(sensorDevice.value)}` : `#${config.value.temperatureSensorDeviceId || '—'}`,
)
const switchLabel = computed(() =>
  switchDevice.value ? `${deviceRecordName(switchDevice.value)} #${deviceRecordId(switchDevice.value)}` : `#${config.value.switchDeviceId || '—'}`,
)
const statusTone = computed(() => Thermostat.outputTone(deviceRecordEffectiveStatus(props.device)))
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
const hysteresisText = computed(() => `${config.value.hysteresisCelsius.toFixed(1)}°C`)
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
