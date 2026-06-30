<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.spiHost')" :model-value="config.host" readonly /></v-col>
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.spiSckPin')" :model-value="config.sckPin" readonly /></v-col>
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.spiMosiPin')" :model-value="config.mosiPin" readonly /></v-col>
        <v-col cols="12" md="6"><v-text-field :label="t('device.fields.spiMisoPin')" :model-value="config.misoPin" readonly /></v-col>
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
        <v-btn color="secondary" variant="tonal" :disabled="busy" @click="emitResetDiagnostics">
          {{ t('device.dialog.busDiagnosticsReset') }}
        </v-btn>
        <v-chip color="secondary" variant="tonal">
          {{ t('device.dialog.spiGeneration', { value: runtime.generation ?? 0 }) }}
        </v-chip>
        <v-chip :color="runtime.transactionActive ? 'warning' : 'secondary'" variant="tonal">
          {{ runtime.transactionActive ? t('device.dialog.spiTransactionActive') : t('device.dialog.spiTransactionIdle') }}
        </v-chip>
      </div>
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
  SpiBusRuntimeSnapshot,
} from '@/api/contracts'
import { SpiBusDevice } from '@/models/devices/spi-bus'

const props = defineProps<{
  device: DeviceRecord
  busy?: boolean
}>()

const emit = defineEmits<{
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()
const config = computed(() => new SpiBusDevice().normalizeConfig(props.device.config))
const runtime = computed<SpiBusRuntimeSnapshot>(() => (props.device.runtime as SpiBusRuntimeSnapshot) ?? {})
const diagnostics = computed<BusRuntimeDiagnosticsSnapshot>(() => runtime.value.diagnostics ?? {})

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
</style>
