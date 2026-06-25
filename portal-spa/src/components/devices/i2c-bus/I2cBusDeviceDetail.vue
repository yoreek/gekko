<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.i2cSdaPin')" :model-value="config.sdaPin" readonly />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.i2cSclPin')" :model-value="config.sclPin" readonly />
        </v-col>
        <v-col cols="12" md="6">
          <v-switch :label="t('device.fields.internalPullup')" :model-value="config.internalPullup" readonly />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field :label="t('device.fields.i2cFrequency')" :model-value="config.frequencyHz" readonly />
        </v-col>
      </v-row>
    </section>

    <section class="device-type-section">
      <div class="device-type-section__heading text-overline">{{ t('device.dialog.i2cRuntimeTitle') }}</div>
      <div class="device-type-section__actions">
        <v-chip color="primary" variant="tonal">
          {{ t('device.dialog.i2cGeneration', { value: runtime.generation ?? 0 }) }}
        </v-chip>
        <v-chip :color="runtime.transactionActive ? 'warning' : 'secondary'" variant="tonal">
          {{ runtime.transactionActive ? t('device.dialog.i2cTransactionActive') : t('device.dialog.i2cTransactionIdle') }}
        </v-chip>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type { DeviceRecord, I2cBusRuntimeSnapshot } from '@/api/contracts'
import { I2cBus } from '@/models/devices/i2c-bus'

const deviceModel = new I2cBus.Device()

const props = defineProps<{
  device: DeviceRecord
  busy?: boolean
}>()

const { t } = useI18n()
const config = computed(() => deviceModel.normalizeConfig(props.device.config))
const runtime = computed<I2cBusRuntimeSnapshot>(() => (props.device.runtime as I2cBusRuntimeSnapshot) ?? {})
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

.device-type-section__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
}
</style>
