<template>
  <div class="device-type-stack">
    <section class="device-type-section">
      <v-row class="device-type-section__grid">
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.i2cSdaPin')"
            :hint="t('device.dialog.common.i2cSdaHint')"
            persistent-hint
            :model-value="currentValue.sdaPin"
            :rules="sdaRules"
            inputmode="numeric"
            type="number"
            min="0"
            max="39"
            :disabled="busy"
            @update:model-value="updateNumber('sdaPin', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.i2cSclPin')"
            :hint="t('device.dialog.common.i2cSclHint')"
            persistent-hint
            :model-value="currentValue.sclPin"
            :rules="sclRules"
            inputmode="numeric"
            type="number"
            min="0"
            max="39"
            :disabled="busy"
            @update:model-value="updateNumber('sclPin', $event)"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-switch
            :label="t('device.fields.internalPullup')"
            :model-value="currentValue.internalPullup"
            :disabled="busy"
            inset
            @update:model-value="update('internalPullup', Boolean($event))"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-text-field
            v-select-on-focus
            :label="t('device.fields.i2cFrequency')"
            :hint="t('device.dialog.common.i2cFrequencyHint')"
            persistent-hint
            :model-value="currentValue.frequencyHz"
            :rules="frequencyRules"
            inputmode="numeric"
            type="number"
            min="1"
            max="400000"
            :disabled="busy"
            @update:model-value="updateNumber('frequencyHz', $event)"
          />
        </v-col>
      </v-row>
    </section>

    <section v-if="device" class="device-type-section">
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
import type { I2cBusConfigDraft, I2cBusCreateDraft } from '@/models/devices/i2c-bus'
import { I2cBusDevice } from '@/models/devices/i2c-bus'

type I2cBusFormValue = I2cBusCreateDraft | I2cBusConfigDraft

const deviceModel = new I2cBusDevice()

const props = defineProps<{
  modelValue: I2cBusFormValue | undefined
  device?: DeviceRecord
  mode?: 'create' | 'edit'
  busy?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: I2cBusFormValue]
  command: [payload: DeviceCommandRequest]
}>()

const { t } = useI18n()

const fallbackValue: I2cBusFormValue = {
  name: 'New Device',
  enabled: true,
  deps: [],
  sdaPin: 21,
  sclPin: 22,
  internalPullup: true,
  frequencyHz: 100000,
}

const currentValue = computed<I2cBusFormValue>(() => props.modelValue ?? fallbackValue)
const sdaRules = computed(() => buildPinRules('sdaPin'))
const sclRules = computed(() => buildPinRules('sclPin'))
const frequencyRules = computed(() => [
  (value: unknown) => {
    const numeric = Number(value)
    return Number.isFinite(numeric) && numeric > 0 || t('device.dialog.i2cFrequencyTooLow')
  },
  (value: unknown) => {
    const numeric = Number(value)
    return Number.isFinite(numeric) && numeric <= 400000 || t('device.dialog.i2cFrequencyTooHigh')
  },
])

const runtime = computed<I2cBusRuntimeSnapshot>(() => {
  if (!props.device) return {}
  return (props.device.runtime as I2cBusRuntimeSnapshot) ?? {}
})

const diagnostics = computed<BusRuntimeDiagnosticsSnapshot>(() => runtime.value.diagnostics ?? {})

const scan = computed<I2cBusScanSnapshot>(() => runtime.value.scan ?? {
  inProgress: false,
  ready: false,
  deviceCount: 0,
  truncated: false,
  nextAddress: 0x08,
  devices: [],
})

function update<K extends keyof I2cBusCreateDraft>(key: K, value: I2cBusCreateDraft[K]): void {
  emit('update:modelValue', buildNextValue({ [key]: value } as Partial<I2cBusCreateDraft>))
}

function updateNumber(key: 'sdaPin' | 'sclPin' | 'frequencyHz', value: string | number): void {
  if (value === '' || value === null || value === undefined) {
    return
  }
  const numeric = Number(value)
  if (!Number.isFinite(numeric)) {
    return
  }
  update(key, numeric as never)
}

function buildNextValue(patch: Partial<I2cBusCreateDraft>): I2cBusFormValue {
  return {
    ...(currentValue.value as I2cBusCreateDraft),
    ...patch,
  }
}

function buildPinRules(field: 'sdaPin' | 'sclPin') {
  const otherField = field === 'sdaPin' ? 'sclPin' : 'sdaPin'
  return [
    (value: unknown) => {
      const numeric = Number(value)
      return Number.isFinite(numeric) && numeric >= 0 || t('device.dialog.i2cPinTooLow')
    },
    (value: unknown) => {
      const numeric = Number(value)
      return Number.isFinite(numeric) && numeric <= 39 || t('device.dialog.i2cPinTooHigh')
    },
    (value: unknown) => {
      const numeric = Number(value)
      const otherValue = Number((currentValue.value as I2cBusCreateDraft)[otherField])
      return !Number.isFinite(numeric) || !Number.isFinite(otherValue) || numeric !== otherValue || t('device.dialog.i2cPinsMustDiffer')
    },
  ]
}

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

.device-type-section__heading {
  margin-bottom: 2px;
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
