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
      <div class="device-type-section__heading text-overline">{{ t('device.dialog.busDiagnosticsTitle') }}</div>
      <div class="device-type-section__actions">
        <v-chip :color="diagnostics.status === 'degraded' ? 'warning' : 'success'" variant="tonal">
          {{ t('device.dialog.busDiagnosticsStatus', { value: diagnostics.status ?? 'ok' }) }}
        </v-chip>
        <v-chip color="primary" variant="tonal">
          {{ t('device.dialog.busDiagnosticsConsecutiveErrors', { value: diagnostics.consecutiveErrors ?? 0 }) }}
        </v-chip>
        <v-chip color="primary" variant="tonal">
          {{ t('device.dialog.busDiagnosticsLastErrorCode', { value: diagnostics.lastErrorCode ?? 0 }) }}
        </v-chip>
        <v-chip color="primary" variant="tonal">
          {{ t('device.dialog.busDiagnosticsLastErrorAtMs', { value: diagnostics.lastErrorAtMs ?? 0 }) }}
        </v-chip>
        <v-chip color="primary" variant="tonal">
          {{ t('device.dialog.busDiagnosticsErrorOps', { value: diagnostics.errorOps ?? 0 }) }}
        </v-chip>
      </div>
      <div class="device-type-section__actions">
        <v-btn color="primary" variant="tonal" :loading="busy || scan.inProgress" :disabled="busy || scan.inProgress" @click="emitScan">
          <v-icon class="me-1" icon="refresh" />
          {{ t('device.dialog.i2cScanAction') }}
        </v-btn>
        <v-btn color="secondary" variant="tonal" :disabled="busy" @click="emitResetDiagnostics">
          {{ t('device.dialog.busDiagnosticsReset') }}
        </v-btn>
        <v-chip color="secondary" variant="tonal">
          {{ t('device.dialog.i2cGeneration', { value: runtime.generation ?? 0 }) }}
        </v-chip>
        <v-chip :color="runtime.transactionActive ? 'warning' : 'secondary'" variant="tonal">
          {{ runtime.transactionActive ? t('device.dialog.i2cTransactionActive') : t('device.dialog.i2cTransactionIdle') }}
        </v-chip>
      </div>
      <v-alert v-if="scan.inProgress" type="info" variant="tonal">
        {{ t('device.dialog.i2cScanInProgress') }}
      </v-alert>
      <v-alert v-else-if="scan.ready && scan.deviceCount === 0" type="info" variant="tonal">
        {{ t('device.dialog.i2cScanEmpty') }}
      </v-alert>
      <v-alert v-if="scan.truncated" type="warning" variant="tonal">
        {{ t('device.dialog.i2cScanTruncated') }}
      </v-alert>
      <v-list v-if="scan.deviceCount > 0" density="compact" class="device-type-section__list">
        <v-list-item v-for="entry in scan.devices" :key="entry.addressHex">
          <v-list-item-title class="text-body-1 font-weight-medium">{{ entry.addressHex }}</v-list-item-title>
        </v-list-item>
      </v-list>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

import type {
  BusRuntimeDiagnosticsSnapshot,
  DeviceCommandRequest,
  DeviceRecord,
  I2cBusRuntimeSnapshot,
  I2cBusScanSnapshot,
} from '@/api/contracts'
import { I2cBusDevice } from '@/models/devices/i2c-bus'

const deviceModel = new I2cBusDevice()

const props = defineProps<{
  device: DeviceRecord
  busy?: boolean
}>()

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const config = computed(() => deviceModel.normalizeConfig(props.device.config))
const runtime = computed<I2cBusRuntimeSnapshot>(() => (props.device.runtime as I2cBusRuntimeSnapshot) ?? {})
const diagnostics = computed<BusRuntimeDiagnosticsSnapshot>(() => runtime.value.diagnostics ?? {})
const scan = computed<I2cBusScanSnapshot>(() => runtime.value.scan ?? {
  inProgress: false,
  ready: false,
  deviceCount: 0,
  truncated: false,
  nextAddress: 0x08,
  devices: [],
})

function emitScan(): void {
  emit('command', { command: 'scan' })
}

function emitResetDiagnostics(): void {
  emit('command', { command: 'resetDiagnostics' })
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

.device-type-section__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
}

.device-type-section__list {
  background: transparent;
}
</style>
