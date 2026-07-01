<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-alert v-if="!temperature?.valid" type="warning" variant="tonal">
        {{ t('device.dialog.temperatureUnavailable') }}
      </v-alert>

      <v-row class="device-type-section__grid">
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
            :model-value="temperature?.valid ? String(temperature.measuredAtMs) : ''"
            readonly
          />
        </v-col>
      </v-row>

      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
            <v-text-field
              :label="t('device.fields.onewireDependency')"
              :model-value="dependencyLabel"
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
    </section>

    <section class="device-type-section">
      <v-expansion-panels>
        <v-expansion-panel value="details">
          <v-expansion-panel-title>
            {{ t('device.dialog.configDetails') }}
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <v-row class="device-type-section__grid">
              <v-col cols="12" md="6">
                <v-text-field :label="t('device.fields.resolution')" :model-value="config.resolution" readonly />
              </v-col>
              <v-col cols="12" md="6">
                <v-text-field :label="t('device.fields.temperatureUnit')" :model-value="t(`device.dialog.temperatureUnit.${config.unit}`)" readonly />
              </v-col>
              <v-col cols="12" md="6">
                <v-text-field :label="t('device.fields.pollMs')" :model-value="config.pollMs" readonly />
              </v-col>
              <v-col cols="12" md="6">
                <v-text-field :label="t('device.fields.reportDelta')" :model-value="config.reportDeltaCelsius" readonly />
              </v-col>
              <v-col cols="12" md="6">
                <v-switch :label="t('device.fields.reportAlways')" :model-value="config.reportAlways" readonly />
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

import type { DeviceRecord, Ds18b20TemperatureSensorOutputSnapshot, TemperatureOutputSnapshot } from '@/api/contracts'
import { Ds18b20Device } from '@/models/devices/ds18b20'
import { useDeviceRegistryStore } from '@/stores/deviceRegistry'

const deviceModel = new Ds18b20Device()

const props = defineProps<{
  device: DeviceRecord
  busy?: boolean
}>()

const { t } = useI18n()
const deviceStore = useDeviceRegistryStore()
const dependencyDeviceId = computed(() => props.device.config.deps.find(dep => dep.role === 'onewire_bus')?.deviceId ?? 0)
const config = computed(() => deviceModel.normalizeConfig(props.device.config, props.device.config.deps))
const output = computed(() => (props.device.runtime as { output?: Ds18b20TemperatureSensorOutputSnapshot }).output)
const temperature = computed(() => output.value?.temperature as TemperatureOutputSnapshot | undefined)
const temperatureText = computed(() => temperature.value?.valid ? `${temperature.value.value.toFixed(2)} ${temperature.value.unitSymbol}` : t('device.dialog.temperatureUnavailableShort'))
const dependencyLabel = computed(() => {
  const dependency = deviceStore.devices.find(device => device.record.id === dependencyDeviceId.value)
  return dependency ? `${dependency.config.name} #${dependency.record.id}` : `#${dependencyDeviceId.value}`
})
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
