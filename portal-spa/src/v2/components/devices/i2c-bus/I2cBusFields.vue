<template>
  <div class="d-flex flex-column ga-4">
    <v-row>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.i2cSdaPin')"
          :hint="t('device.dialog.i2cSdaHint')"
          persistent-hint
          :model-value="modelValue.sdaPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('sdaPin', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.i2cSclPin')"
          :hint="t('device.dialog.i2cSclHint')"
          persistent-hint
          :model-value="modelValue.sclPin"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('sclPin', Number($event))"
        />
      </v-col>

      <v-col cols="12" sm="6">
        <v-text-field
          type="number"
          :label="t('device.fields.i2cFrequency')"
          :hint="t('device.dialog.i2cFrequencyHint')"
          persistent-hint
          :model-value="modelValue.frequencyHz"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          @update:model-value="update('frequencyHz', Number($event))"
        />
      </v-col>
      <v-col cols="12" sm="6">
        <v-switch
          :label="t('device.fields.internalPullup')"
          :model-value="modelValue.internalPullup"
          :readonly="mode === 'view'"
          :disabled="busy && mode !== 'view'"
          inset
          @update:model-value="update('internalPullup', Boolean($event))"
        />
      </v-col>
    </v-row>

    <div v-if="device">
      <div class="text-overline text-medium-emphasis mb-2">{{ t('device.dialog.busDiagnosticsTitle') }}</div>

      <div class="d-flex flex-wrap ga-2 mb-3">
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

      <div class="d-flex flex-wrap align-center ga-2 mb-3">
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

      <v-alert v-if="scan.inProgress" type="info" variant="tonal" class="mb-2">
        {{ t('device.dialog.i2cScanInProgress') }}
      </v-alert>
      <v-alert v-else-if="scan.ready && scan.deviceCount === 0" type="info" variant="tonal" class="mb-2">
        {{ t('device.dialog.i2cScanEmpty') }}
      </v-alert>
      <v-alert v-if="scan.truncated" type="warning" variant="tonal" class="mb-2">
        {{ t('device.dialog.i2cScanTruncated') }}
      </v-alert>

      <v-list v-if="scan.deviceCount > 0" density="compact">
        <v-list-item v-for="entry in scan.devices" :key="entry.addressHex">
          <v-list-item-title class="font-weight-medium">{{ entry.addressHex }}</v-list-item-title>
        </v-list-item>
      </v-list>
    </div>
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
import type { I2cBusConfigDraft } from '@/models/devices/i2c-bus'

const props = defineProps<{
  modelValue: I2cBusConfigDraft
  device?: DeviceRecord
  mode: 'view' | 'edit' | 'create'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: I2cBusConfigDraft]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()

const runtime = computed<I2cBusRuntimeSnapshot>(() => (props.device?.runtime as I2cBusRuntimeSnapshot) ?? {})
const diagnostics = computed<BusRuntimeDiagnosticsSnapshot>(() => runtime.value.diagnostics ?? {})
const scan = computed<I2cBusScanSnapshot>(() => runtime.value.scan ?? {
  inProgress: false,
  ready: false,
  deviceCount: 0,
  truncated: false,
  nextAddress: 0x08,
  devices: [],
})

function update<K extends keyof I2cBusConfigDraft>(key: K, value: I2cBusConfigDraft[K]): void {
  emit('update:modelValue', { ...props.modelValue, [key]: value })
}

function emitScan(): void {
  emit('command', { command: 'scan' })
}

function emitResetDiagnostics(): void {
  emit('command', { command: 'resetDiagnostics' })
}
</script>
