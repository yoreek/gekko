<template>
  <div class="d-grid ga-3">
    <v-alert v-if="!temperature?.valid" type="warning" variant="tonal">
      {{ t('device.dialog.temperatureUnavailable') }}
    </v-alert>

    <v-row>
      <v-col cols="12" md="6">
        <v-text-field
          :label="t('device.fields.temperature')"
          :model-value="temperatureText"
          readonly
        />
      </v-col>
      <v-col cols="12" md="6">
        <v-text-field
          :label="t('device.fields.measuredAt')"
          :model-value="temperature?.valid ? String(temperature.measured_at_ms) : ''"
          readonly
        />
      </v-col>
    </v-row>

    <v-row>
      <v-col cols="12" md="6">
        <v-text-field
          :label="t('device.fields.onewireParent')"
          :model-value="parentLabel"
          readonly
        />
      </v-col>
      <v-col cols="12" md="6">
        <v-text-field
          :label="t('device.fields.ds18b20Address')"
          :model-value="config.address"
          readonly
        />
      </v-col>
    </v-row>

    <v-expansion-panels>
      <v-expansion-panel value="details">
        <v-expansion-panel-title>
          {{ t('device.dialog.configDetails') }}
        </v-expansion-panel-title>
        <v-expansion-panel-text>
          <v-row>
            <v-col cols="12" md="6">
              <v-text-field :label="t('device.fields.resolution')" :model-value="config.resolution" readonly />
            </v-col>
            <v-col cols="12" md="6">
              <v-text-field :label="t('device.fields.temperatureUnit')" :model-value="t(`device.dialog.temperatureUnit.${config.unit}`)" readonly />
            </v-col>
            <v-col cols="12" md="6">
              <v-text-field :label="t('device.fields.pollMs')" :model-value="config.poll_ms" readonly />
            </v-col>
            <v-col cols="12" md="6">
              <v-text-field :label="t('device.fields.reportDelta')" :model-value="config.report_delta_celsius" readonly />
            </v-col>
            <v-col cols="12" md="6">
              <v-switch :label="t('device.fields.reportAlways')" :model-value="config.report_always" readonly />
            </v-col>
          </v-row>
        </v-expansion-panel-text>
      </v-expansion-panel>
    </v-expansion-panels>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DashboardDevice } from '@/models/device'
import {
  formatTemperatureOutput,
  normalizeDs18b20TemperatureSensorConfig,
  temperatureOutputValid,
} from '@/models/devices/ds18b20'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const props = defineProps<{
  device: DashboardDevice
  busy?: boolean
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const config = computed(() => normalizeDs18b20TemperatureSensorConfig(props.device.detail.config, props.device.parentDeviceId))
const temperature = computed(() => {
  const value = props.device.output.temperature
  return temperatureOutputValid(value) ? value : undefined
})
const temperatureText = computed(() => formatTemperatureOutput(temperature.value) || t('device.dialog.temperatureUnavailableShort'))
const parentLabel = computed(() => {
  const parent = deviceStore.devices.find(device => device.deviceId === props.device.parentDeviceId)
  return parent ? `${parent.name} #${parent.deviceId}` : `#${props.device.parentDeviceId}`
})
</script>
